import cv2
import os
import glob

# Ajouts pour la détection TFLite
from functools import lru_cache
from pathlib import Path
from typing import List, Dict, Tuple, Optional
import numpy as np

import struct

# Exemple de point (x, y)
class Point:
    def __init__(self, x: int, y: int):
        self.x = int(x)
        self.y = int(y)
        self.z = 0
        self.theta = 0.0

class Obstacle:
    def __init__(self, type_id: int, pointd: Point, pointg: Point, pointg_x: int, pointg_y: int):
        self.type_id = type_id
        self.pointd = pointd
        self.pointg = pointg

# Charger l'interpréteur TFLite (tflite-runtime sur Linux/ARM, sinon tensorflow.lite)
try:
    import tflite_runtime.interpreter as tflite
except Exception:
    # macOS / dev : utiliser TensorFlow qui expose tf.lite.Interpreter
    from tensorflow import lite as tflite


def _nms(boxes, scores, iou_thresh=0.45, top_k=300):
    if len(boxes) == 0:
        return []
    x1 = boxes[:, 0].astype(np.float32)
    y1 = boxes[:, 1].astype(np.float32)
    x2 = boxes[:, 2].astype(np.float32)
    y2 = boxes[:, 3].astype(np.float32)
    areas = (x2 - x1 + 1) * (y2 - y1 + 1)
    order = scores.argsort()[::-1]
    keep = []
    while order.size > 0 and len(keep) < top_k:
        i = order[0]
        keep.append(int(i))
        xx1 = np.maximum(x1[i], x1[order[1:]])
        yy1 = np.maximum(y1[i], y1[order[1:]])
        xx2 = np.minimum(x2[i], x2[order[1:]])
        yy2 = np.minimum(y2[i], y2[order[1:]])
        w = np.maximum(0.0, xx2 - xx1 + 1)
        h = np.maximum(0.0, yy2 - yy1 + 1)
        inter = w * h
        ovr = inter / (areas[i] + areas[order[1:]] - inter + 1e-9)
        inds = np.where(ovr <= iou_thresh)[0]
        order = order[inds + 1]
    return keep


def _xywh2xyxy(xywh):
    # xywh -> xyxy
    x, y, w, h = xywh.T
    x1 = x - w / 2
    y1 = y - h / 2
    x2 = x + w / 2
    y2 = y + h / 2
    return np.stack([x1, y1, x2, y2], axis=1)

def capture_image_from_source(source):
    """
    Ouvre une source vidéo (webcam ou fichier) et récupère une image.
    Args:
        source (int | str): index de webcam (0, 1, ...) ou chemin vers un fichier vidéo/image.
    Returns:
        np.ndarray: image BGR capturée.
    Raises:
        RuntimeError: si la source ne peut pas être ouverte ou si aucune image n'est lue.
    """
    # Ouvre la source
    if isinstance(source, int) or (isinstance(source, str) and source.isdigit()):
        cap = cv2.VideoCapture(int(source))
    else:
        cap = cv2.VideoCapture(source)

    if not cap.isOpened():
        raise RuntimeError(f"Impossible d'ouvrir la source : {source}")

    ok, frame = cap.read()
    cap.release()

    if not ok or frame is None:
        raise RuntimeError(f"Impossible de lire une image depuis la source : {source}")

    return frame

def _list_tflite_candidates(max_n=8):
    candidates = []
    search_roots = [Path.cwd(), Path(__file__).resolve().parent, Path(__file__).resolve().parent.parent]
    for root in search_roots:
        for sub in (root, root / 'models', root / 'IA', root / 'IA' / 'models'):
            try:
                for p in sub.glob('**/*.tflite'):
                    candidates.append(str(p))
            except Exception:
                pass
    # Dédoublonner en conservant l'ordre
    seen = set()
    uniq = []
    for c in candidates:
        if c not in seen:
            uniq.append(c)
            seen.add(c)
    return uniq[:max_n]

# --- Détecteur YOLOv8 TFLite autonome ---
class TFLiteYOLO:
    """Détecteur YOLOv8 exporté en TFLite (sans NMS intégré), autonome.
    Attend typiquement une sortie de forme (1, 84, N) ou (1, N, 84):
      - 4 premières valeurs = bbox (xywh) en pixels de l'image redimensionnée
      - 80 suivantes = scores de classes (après sigmoïde)
    """
    def __init__(self, weights: Path, imgsz: int = 640, conf: float = 0.25, iou: float = 0.45, threads: int = 2):
        # Résolution robuste du chemin des poids
        w = Path(weights).expanduser()
        if not w.exists():
            # Essayer relatif au fichier courant (IA/) et à la racine du projet
            here = Path(__file__).resolve().parent
            for base in [here, here.parent, Path.cwd()]:
                cand = (base / w)
                if cand.exists():
                    w = cand
                    break
        if not w.exists():
            # Dernière chance: si on a donné un dossier, chercher un .tflite dedans
            if w.is_dir():
                tfs = list(w.glob('*.tflite'))
                if tfs:
                    w = tfs[0]
        if not w.exists():
            tips = _list_tflite_candidates()
            tip_text = ("\nSuggestions .tflite trouvées:\n- " + "\n- ".join(tips)) if tips else "\n(Aucun .tflite trouvé automatiquement)"
            raise FileNotFoundError(f"Modèle TFLite introuvable: {weights}{tip_text}")
        self.weights = w
        self.imgsz = int(imgsz)
        self.conf = float(conf)
        self.iou = float(iou)
        try:
            self.interpreter = tflite.Interpreter(model_path=str(self.weights), num_threads=int(threads))
        except TypeError:
            self.interpreter = tflite.Interpreter(model_path=str(self.weights))
        self.interpreter.allocate_tensors()
        self.inp = self.interpreter.get_input_details()[0]
        self.out = self.interpreter.get_output_details()[0]
        # Taille entrée du modèle
        self.in_h = int(self.inp['shape'][1])
        self.in_w = int(self.inp['shape'][2])

    def _preprocess(self, bgr):
        # letterbox simple: redimension isotrope + padding noir pour conserver le ratio
        h0, w0 = bgr.shape[:2]
        r = min(self.in_w / w0, self.in_h / h0)
        nw, nh = int(round(w0 * r)), int(round(h0 * r))
        resized = cv2.resize(bgr, (nw, nh), interpolation=cv2.INTER_LINEAR)
        canvas = np.zeros((self.in_h, self.in_w, 3), dtype=np.uint8)
        dw, dh = (self.in_w - nw) // 2, (self.in_h - nh) // 2
        canvas[dh:dh+nh, dw:dw+nw] = resized
        # Normalisation 0-1 et NHWC->NCHW si besoin
        inp = canvas.astype(np.float32) / 255.0
        if self.inp['shape'][3] == 3:  # NHWC
            inp = np.expand_dims(inp, axis=0)
        else:  # NCHW
            inp = np.transpose(inp, (2, 0, 1))[None]
        meta = {
            'r': r,
            'dw': dw,
            'dh': dh,
            'orig_w': w0,
            'orig_h': h0,
            'in_w': self.in_w,
            'in_h': self.in_h,
        }
        return inp, meta

    def _postprocess(self, out_arr, meta, class_filter=None):
        # Tolérer (1, 84, N) ou (1, N, 84)
        out = out_arr.squeeze()
        if out.ndim != 2:
            return []
        c1, c2 = out.shape
        if c1 == 84:  # (84, N)
            out = out.T  # -> (N, 84)
        elif c2 == 84:
            pass  # (N, 84)
        else:
            # format inconnu
            return []
        xywh = out[:, :4]
        # Certaines exports TFLite renvoient des coordonnees normalisees [0,1]
        # Si toutes les valeurs sont <= 1.5, on remultiplie par la taille d'entree du modele
        if np.all(xywh <= 1.5):
            xywh[:, 0] *= meta.get('in_w', self.in_w)
            xywh[:, 1] *= meta.get('in_h', self.in_h)
            xywh[:, 2] *= meta.get('in_w', self.in_w)
            xywh[:, 3] *= meta.get('in_h', self.in_h)
        cls_scores = out[:, 4:]
        # Si export a déjà activations, on suppose [0,1]; sinon appliquer sigmoïde
        if cls_scores.max() > 1.0 or cls_scores.min() < 0.0:
            cls_scores = 1.0 / (1.0 + np.exp(-cls_scores))
        scores = cls_scores.max(axis=1)
        cls_ids = cls_scores.argmax(axis=1)
        # Seuil de confiance
        m = scores >= self.conf
        if class_filter is not None:
            class_filter = set(int(c) for c in class_filter)
            m &= np.array([int(c) in class_filter for c in cls_ids])
        xywh = xywh[m]
        scores = scores[m]
        cls_ids = cls_ids[m]
        if xywh.size == 0:
            return []
        # Convertir vers l'image originale (déletterbox)
        boxes = _xywh2xyxy(xywh)
        boxes[:, [0, 2]] -= meta['dw']
        boxes[:, [1, 3]] -= meta['dh']
        boxes /= meta['r']
        # Clamp
        boxes[:, 0] = np.clip(boxes[:, 0], 0, meta['orig_w'] - 1)
        boxes[:, 1] = np.clip(boxes[:, 1], 0, meta['orig_h'] - 1)
        boxes[:, 2] = np.clip(boxes[:, 2], 0, meta['orig_w'] - 1)
        boxes[:, 3] = np.clip(boxes[:, 3], 0, meta['orig_h'] - 1)
        # NMS par classe
        final_dets = []
        for c in np.unique(cls_ids):
            idxs = np.where(cls_ids == c)[0]
            keep = _nms(boxes[idxs], scores[idxs], iou_thresh=self.iou)
            for k in keep:
                i = idxs[k]
                x1, y1, x2, y2 = boxes[i]
                final_dets.append((int(x1), int(y1), int(x2), int(y2), float(scores[i]), int(c)))
        # Trier par score décroissant
        final_dets.sort(key=lambda d: d[4], reverse=True)
        return final_dets

    def infer(self, bgr, class_filter=None):
        inp, meta = self._preprocess(bgr)
        self.interpreter.set_tensor(self.inp['index'], inp)
        self.interpreter.invoke()
        out = self.interpreter.get_tensor(self.out['index'])
        return self._postprocess(out, meta, class_filter=class_filter)

# --- Détection d'objets avec TFLiteYOLO sur image unique ---

@lru_cache(maxsize=3)
def _load_tflite_detector_cached(weights_str: str, imgsz: int, conf: float, iou: float, threads: int):
    weights = Path(weights_str)
    return TFLiteYOLO(weights=weights, imgsz=imgsz, conf=conf, iou=iou, threads=threads)


def _normalize_classes_arg(classes) -> Optional[List[int]]:
    if classes is None or classes == '':
        return None
    if isinstance(classes, (list, tuple)):
        return [int(c) for c in classes]
    if isinstance(classes, str):
        return [int(c.strip()) for c in classes.split(',') if c.strip() != '']
    raise TypeError("Argument 'classes' doit être une liste d'entiers, un tuple, une chaîne '0,1', ou None.")


def detect_objects_tflite_from_image(
    image_bgr: np.ndarray,
    weights: str | Path,
    imgsz: int = 640,
    conf: float = 0.25,
    iou: float = 0.45,
    threads: int = 2,
    classes=None
) -> List[Dict[str, float]]:
    if not isinstance(image_bgr, np.ndarray):
        raise TypeError("image_bgr doit être un np.ndarray (image OpenCV BGR)")
    
    weights_path = str(Path(weights))
    detector = _load_tflite_detector_cached(weights_path, int(imgsz), float(conf), float(iou), int(threads))

    class_filter = _normalize_classes_arg(classes)

    dets = detector.infer(image_bgr, class_filter=class_filter)

    results: List[Dict[str, float]] = []
    for (x1, y1, x2, y2, score, cid) in dets:
        results.append({
            "class_id": int(cid),
            "score": float(score),
            "x1": int(x1),
            "y1": int(y1),
            "x2": int(x2),
            "y2": int(y2),
        })
    return results

# Exemple d'utilisation:
img = capture_image_from_source(0)
objs = detect_objects_tflite_from_image(img,"models/yolov8n_saved_model/yolov8n_float32.tflite",imgsz=640, conf=0.25, threads=3)
print(objs)
print("nb det:", len(objs))

def transformationPixelVersCoordonnees(det):
    #detsExemple=[{'class_id': 0, 'score': 0.7719968557357788, 'x1': 219, 'y1': 181, 'x2': 1653, 'y2': 1069}]
    distance=CalculeDistanceFromPixel(det) # À définir
    left=CalculeYFromPixel(det) # À définir
    right=CalculeYFromPixel(det) # À définir
    Obstacle = Obstacle( type_id=det["class_id"], pointd=Point(distance,left), pointd=Point(distance,right))
    return Obstacle

def draw_detections_on_image(img: np.ndarray, objs: List[Dict[str, float]]) -> None:
    for det in objs:
        x1, y1, x2, y2 = det["x1"], det["y1"], det["x2"], det["y2"]
        class_id = det["class_id"]
        score = det["score"]

        # Sauter les boites degeneres (quasi nulles)
        if (x2 - x1) < 2 or (y2 - y1) < 2:
            continue

        # Dessiner un rectangle vert
        cv2.rectangle(img, (x1, y1), (x2, y2), (0, 255, 0), 2)
        # Ajouter le label au-dessus de la boite
        label = f"ID {class_id}: {score:.2f}"
        cv2.putText(img, label, (x1, max(0, y1 - 10)), cv2.FONT_HERSHEY_SIMPLEX, 0.6, (0, 255, 0), 2)
