#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Détection de panneaux – Version TensorFlow Lite (TFLite)

• N'utilise PAS ultralytics/torch : inférence directe via TensorFlow Lite / tflite-runtime
• Compatible ordinateur (macOS) et Raspberry Pi (recommandé : tflite-runtime)
• Nécessite un modèle YOLOv8 exporté en .tflite (ex: best_int8.tflite)

Installation (venv) :
  # Sur Raspberry Pi (recommandé)
  pip install tflite-runtime opencv-python

  # Sur macOS (dev local) : tflite-runtime n'est pas dispo via pip -> utiliser tf.lite
  pip install tensorflow-macos opencv-python

  # Pour utiliser un .pt (Ultralytics/PyTorch) sur desktop
  pip install ultralytics torch torchvision

Exemples :
  # Webcam avec le modèle TFLite exporté (défaut fourni)
  python3 detectionImage.py --source 0 --img 320 --conf 0.25

  # Vidéo + seuil de confiance + classes (ids séparés par virgules)
  python3 detectionImage.py --source video.mp4 --weights best.tflite --conf 0.35 --img 416 --classes 0,1

  # Desktop (macOS) avec Ultralytics .pt (PyTorch)
  # (nécessite: pip install ultralytics torch torchvision)
  python3 detectionImage.py --source 0 \
    --weights yolov8n.pt --img 640 --conf 0.25 --device mps

Touches :
  q -> quitter
  p -> pause/reprendre
"""

import argparse
from pathlib import Path
import time
import sys

import cv2
import numpy as np

# ---- Import TFLite : privilégier tflite-runtime, sinon fallback vers tf.lite ----
try:
    import tflite_runtime.interpreter as tflite
except Exception:  # macOS etc.
    try:
        import tensorflow as tf
        tflite = tf.lite  # fallback
    except Exception as e:
        print("\n[ERREUR] Ni tflite-runtime ni tensorflow ne sont installés.\n"
              "Installe : 'pip install tflite-runtime' (Linux/ARM) ou 'pip install tensorflow-macos' (macOS).\n")
        raise e

# Optionnel: support Ultralytics (.pt) pour l'inférence PyTorch sur desktop
from ultralytics import YOLO as _YOLO


# Modèle TFLite exporté localement par défaut
DEFAULT_TFLITE_WEIGHTS = Path(__file__).resolve().parent.parent / "models" / "best_saved_model" / "best_float32.tflite"

IMAGE_EXTENSIONS = {'.jpg', '.jpeg', '.png', '.bmp', '.tif', '.tiff', '.webp'}

CLASS_NAMES = {
    0: "obstacle_voiture",
    1: "panneau-limitation30",
    2: "panneau_barriere",
    3: "panneau_ceder_passage",
    4: "panneau_fin30",
    5: "panneau_intersection",
    6: "panneau_parking",
    7: "panneau_sens_unique",
    8: "pont",
}

# ------------------------- Utilitaires -------------------------

def letterbox(img, new_shape=(640, 640), color=(114, 114, 114)):
    """Redimension avec bandes (letterbox) pour garder le ratio.
    Retourne : image, ratio (rw, rh), dwdh (pad_w, pad_h)
    """
    shape = img.shape[:2]  # h, w
    if isinstance(new_shape, int):
        new_shape = (new_shape, new_shape)
    r = min(new_shape[0] / shape[0], new_shape[1] / shape[1])
    new_unpad = (int(round(shape[1] * r)), int(round(shape[0] * r)))  # w, h
    # resize
    img_resized = cv2.resize(img, new_unpad, interpolation=cv2.INTER_LINEAR)
    # padding
    dw = new_shape[1] - new_unpad[0]
    dh = new_shape[0] - new_unpad[1]
    dw /= 2
    dh /= 2
    top, bottom = int(round(dh - 0.1)), int(round(dh + 0.1))
    left, right = int(round(dw - 0.1)), int(round(dw + 0.1))
    img_padded = cv2.copyMakeBorder(img_resized, top, bottom, left, right, cv2.BORDER_CONSTANT, value=color)
    ratio = (r, r)
    dwdh = (dw, dh)
    return img_padded, ratio, dwdh


def nms_boxes(boxes, scores, iou_thres=0.45, score_thres=0.25):
    """NMS en utilisant OpenCV. boxes: [x,y,w,h] en pixels de l'image letterboxed."""
    idxs = cv2.dnn.NMSBoxes(boxes, scores, score_thres, iou_thres)
    if len(idxs) == 0:
        return []
    # OpenCV peut renvoyer un tableau d'indices Nx1
    return [int(i) for i in np.array(idxs).reshape(-1)]


def parse_class_filter(arg):
    """Retourne une liste d'IDs de classes à garder (ints) ou None pour ne pas filtrer."""
    if not arg:
        return None
    parts = [p.strip() for p in str(arg).split(',') if p.strip()]
    if not parts:
        return None
    if all(p.isdigit() for p in parts):
        return [int(p) for p in parts]
    print("[AVERTISSEMENT] --classes doit contenir des IDs entiers (ex: '0,1'). Filtre ignoré.")
    return None


# ------------------------- Inférence TFLite -------------------------

class TFLiteYOLO:
    def __init__(self, weights: Path, imgsz: int = 640, conf: float = 0.25, iou: float = 0.45, threads: int = 1):
        weights = Path(weights)
        if not weights.exists():
            raise FileNotFoundError(f"Poids TFLite introuvables: {weights}")
        self.imgsz = int(imgsz)
        self.conf = float(conf)
        self.iou = float(iou)
        self.debug_raw = False

        self.interpreter = tflite.Interpreter(model_path=str(weights))
        self.interpreter.allocate_tensors()
        self.inp = self.interpreter.get_input_details()[0]
        self.output_details = self.interpreter.get_output_details()
        self.out = self.output_details[0]

        # Déduire format d'entrée
        self.in_h, self.in_w = self.inp['shape'][1], self.inp['shape'][2]
        # Certains exports YOLO ignorent self.imgsz; on force à la taille d'entrée du modèle
        self.imgsz = int(self.in_w)
        self.num_classes = len(CLASS_NAMES)
        # Essayer de déduire le nombre de classes réel à partir de la sortie
        out_shape = tuple(int(s) for s in self.out.get('shape', ()))
        feature_dims = [d for d in out_shape if d not in (0, 1) and d > 4]
        if feature_dims:
            inferred = feature_dims[0] - 4
            if inferred > 0:
                self.num_classes = max(self.num_classes, inferred)

    def infer(self, bgr, class_filter=None):
        h0, w0 = bgr.shape[:2]
        img, ratio, dwdh = letterbox(bgr, (self.imgsz, self.imgsz))
        x = cv2.cvtColor(img, cv2.COLOR_BGR2RGB).astype(np.float32) / 255.0
        x = np.expand_dims(x, 0)

        self.interpreter.set_tensor(self.inp['index'], x)
        self.interpreter.invoke()

        raw_outputs = [
            np.squeeze(self.interpreter.get_tensor(detail['index']))
            for detail in self.output_details
        ]

        # Gestion des exports à sorties multiples (boxes + scores)
        if len(raw_outputs) >= 2:
            xywh = raw_outputs[0]
            cls_raw = raw_outputs[1]
            xywh = np.reshape(xywh, (-1, xywh.shape[-1]))
            cls_raw = np.reshape(cls_raw, (-1, cls_raw.shape[-1]))
            if xywh.shape[1] != 4:
                raise RuntimeError(f"Sortie boxes inattendue: shape={xywh.shape}")
            obj_raw = None
        else:
            pred = raw_outputs[0]
            if pred.ndim == 3:
                pred = pred.reshape(pred.shape[0], -1)
            pred = np.squeeze(pred)
            if pred.ndim != 2:
                raise RuntimeError(f"Sortie inattendue du modèle TFLite: shape={pred.shape}")

            if self.debug_raw and not hasattr(self, '_debug_dumped'):
                print(f"[DEBUG] TFLite raw shape={pred.shape}, min={pred.min():.4f}, max={pred.max():.4f}")
                print("[DEBUG] Exemple (5 premières lignes):")
                for row in pred[:5]:
                    print("  ", " ".join(f"{v:0.3f}" for v in row[:10]))
                self._debug_dumped = True

            if pred.shape[1] not in {4 + self.num_classes, 5 + self.num_classes}:
                if pred.shape[0] in {4 + self.num_classes, 5 + self.num_classes}:
                    pred = pred.T

            nc = self.num_classes or max(0, pred.shape[1] - 4)
            if pred.shape[1] == (4 + nc):
                xywh = pred[:, :4]
                cls_raw = pred[:, 4:]
                obj_raw = None
            elif pred.shape[1] == (5 + nc):
                xywh = pred[:, :4]
                obj_raw = pred[:, 4:5]
                cls_raw = pred[:, 5:]
            else:
                xywh = pred[:, :4]
                cls_raw = pred[:, 4:]
                obj_raw = None
        # Debug list for multiple-output branch
        if len(raw_outputs) >= 2 and self.debug_raw and not hasattr(self, '_debug_dumped'):
            print(f"[DEBUG] TFLite boxes shape={xywh.shape}, scores shape={cls_raw.shape}")
            self._debug_dumped = True

        # Mise à l'échelle des scores : certains exports renvoient des logits bruts
        if cls_raw.size == 0:
            return []
        if cls_raw.max() > 1.0 or cls_raw.min() < 0.0:
            cls_scores = 1.0 / (1.0 + np.exp(-cls_raw))
        else:
            cls_scores = cls_raw

        # Conserver objectness si disponible
        if 'obj_raw' in locals() and obj_raw is not None:
            if obj_raw.max() > 1.0 or obj_raw.min() < 0.0:
                obj_scores = 1.0 / (1.0 + np.exp(-obj_raw))
            else:
                obj_scores = obj_raw
            cls_scores = cls_scores * obj_scores

        scores = cls_scores.max(axis=1)
        class_probs = cls_scores
        class_ids = class_probs.argmax(axis=1)

        # Filtrer par confiance
        keep = scores >= self.conf
        xywh = xywh[keep]
        scores = scores[keep]
        class_probs = class_probs[keep]
        class_ids = class_ids[keep]

        # Filtre de classes
        if class_filter is not None:
            m = np.isin(class_ids, np.array(class_filter, dtype=int))
            xywh = xywh[m]
            scores = scores[m]
            class_probs = class_probs[m]
            class_ids = class_ids[m]

        # Convertir xywh (sur image padded) -> xyxy pixels
        # Les coordonnées sont relatives à l'image d'entrée (imgsz)
        # YOLO export renvoie souvent en pixels sur l'entrée; sinon normalisées [0,1].
        # On détecte en supposant que max coord > 1 => déjà en pixels
        if xywh.size:
            if np.all(xywh <= 1.5):
                xywh_px = xywh.copy()
                xywh_px[:, 0] *= float(self.in_w)
                xywh_px[:, 1] *= float(self.in_h)
                xywh_px[:, 2] *= float(self.in_w)
                xywh_px[:, 3] *= float(self.in_h)
            else:
                xywh_px = xywh
        else:
            xywh_px = xywh
        x, y, w, h = xywh_px[:, 0], xywh_px[:, 1], xywh_px[:, 2], xywh_px[:, 3]
        x1 = x - w / 2
        y1 = y - h / 2
        x2 = x + w / 2
        y2 = y + h / 2
        # Enlever le padding et re-projeter vers l'image d'origine
        dw, dh = dwdh
        x1 = (x1 - dw) / ratio[0]
        y1 = (y1 - dh) / ratio[1]
        x2 = (x2 - dw) / ratio[0]
        y2 = (y2 - dh) / ratio[1]

        # Clipper aux limites
        x1 = np.clip(x1, 0, w0)
        y1 = np.clip(y1, 0, h0)
        x2 = np.clip(x2, 0, w0)
        y2 = np.clip(y2, 0, h0)

        # Préparer pour NMS OpenCV (x,y,w,h)
        boxes_xywh = np.stack([x1, y1, x2 - x1, y2 - y1], axis=1).tolist()
        scores_list = scores.tolist()

        keep_idx = nms_boxes(boxes_xywh, scores_list, iou_thres=self.iou, score_thres=self.conf)

        detections = []  # liste de tuples (x1,y1,x2,y2,score,cls_id)
        for i in keep_idx:
            xx1, yy1, ww, hh = boxes_xywh[i]
            cls_id = int(class_ids[i]) if class_ids.size else 0
            detections.append((int(xx1), int(yy1), int(xx1 + ww), int(yy1 + hh), float(scores_list[i]), cls_id))
        return detections


# ------------------------- Inférence Ultralytics (.pt) -------------------------

class YOLOv8PT:
    def __init__(self, weights: Path, imgsz: int = 640, conf: float = 0.25, iou: float = 0.45, device: str | None = None):
        if _YOLO is None:
            raise ImportError("Ultralytics non installé. Fais: 'pip install ultralytics'.")
        weights = Path(weights)
        if not weights.exists():
            raise FileNotFoundError(f"Poids .pt introuvables: {weights}")
        self.imgsz = int(imgsz)
        self.conf = float(conf)
        self.iou = float(iou)
        self.device = device  # 'cpu', 'cuda', 'mps', etc.
        self.model = _YOLO(str(weights))

    def infer(self, bgr, class_filter=None):
        # Ultralytics accepte directement un ndarray BGR
        res = self.model.predict(
            bgr,
            imgsz=self.imgsz,
            conf=self.conf,
            iou=self.iou,
            device=self.device if self.device else None,
            verbose=False)
        if not res:
            return []
        r0 = res[0]
        if r0.boxes is None or r0.boxes.shape[0] == 0:
            return []
        boxes = r0.boxes
        # xyxy in pixels, conf, cls
        xyxy = boxes.xyxy.cpu().numpy()
        scores = boxes.conf.cpu().numpy()
        cls_ids = boxes.cls.cpu().numpy().astype(int)
        dets = []
        for (x1, y1, x2, y2), s, c in zip(xyxy, scores, cls_ids):
            if class_filter is not None and int(c) not in class_filter:
                continue
            dets.append((int(x1), int(y1), int(x2), int(y2), float(s), int(c)))
        return dets


def create_detector(weights: Path, imgsz: int, conf: float, iou: float, threads: int, device: str | None):
    ext = str(Path(weights).suffix).lower()
    if ext == '.tflite':
        return TFLiteYOLO(weights=weights, imgsz=imgsz, conf=conf, iou=iou, threads=threads)
    if ext == '.pt':
        return YOLOv8PT(weights=weights, imgsz=imgsz, conf=conf, iou=iou, device=device)
    raise ValueError(f"Extension de poids non supportée: {ext}. Utilise .tflite ou .pt")


def draw_detections(frame: np.ndarray, dets, fps: float | None = None):
    """Dessine les boîtes + labels sur une copie de l'image source."""
    annotated = frame.copy()
    for x1, y1, x2, y2, score, cid in dets:
        cv2.rectangle(annotated, (x1, y1), (x2, y2), (0, 200, 255), 2)
        label_name = CLASS_NAMES.get(cid, f"class_{cid}")
        label = f"{label_name} {score:.2f}"
        (tw, th), _ = cv2.getTextSize(label, cv2.FONT_HERSHEY_SIMPLEX, 0.5, 1)
        cv2.rectangle(annotated, (x1, y1 - th - 6), (x1 + tw + 6, y1), (0, 0, 0), -1)
        cv2.putText(annotated, label, (x1 + 3, y1 - 4), cv2.FONT_HERSHEY_SIMPLEX, 0.5, (255, 255, 255), 1, cv2.LINE_AA)
    if fps is not None:
        cv2.putText(annotated, f"FPS: {fps:5.1f}", (10, 22), cv2.FONT_HERSHEY_SIMPLEX, 0.6, (40, 220, 0), 2, cv2.LINE_AA)
    return annotated


# ------------------------- Programme principal -------------------------

def parse_args():
    ap = argparse.ArgumentParser(description="Détection YOLO TFLite (webcam/vidéo/image)")
    ap.add_argument('--source', default='0', help='Index caméra (0,1,...) ou chemin vidéo/image')
    ap.add_argument(
        '--weights',
        type=Path,
        default=DEFAULT_TFLITE_WEIGHTS,
        help=f'Chemin vers le modèle (.tflite ou .pt). Défaut: {DEFAULT_TFLITE_WEIGHTS}',
    )
    ap.add_argument('--conf', type=float, default=0.25, help='Seuil de confiance')
    ap.add_argument('--img', type=int, default=640, help="Taille d'entrée (côté long)")
    ap.add_argument('--iou', type=float, default=0.45, help='Seuil IOU pour NMS')
    ap.add_argument('--classes', type=str, default='', help="Filtre classes par IDs (ex: '0' ou '0,1')")
    ap.add_argument('--max-fps', type=float, default=0.0, help='Limiter les FPS (0 = illimité)')
    ap.add_argument('--device', type=str, default='', help="Device pour Ultralytics .pt (cpu, cuda, mps)")
    ap.add_argument('--debug-raw', action='store_true', help="Afficher la sortie brute du modèle (diagnostic)")
    ap.add_argument('--no-show', action='store_true', help="Ne pas ouvrir de fenêtre d'affichage (diagnostic/headless)")
    ap.add_argument('--print-dets', action='store_true', help="Afficher les objets détectés dans le terminal")
    return ap.parse_args()


def open_source(src):
    src_str = str(src)
    if src_str.isdigit() and len(src_str) < 3:
        cap = cv2.VideoCapture(int(src_str))
        if not cap.isOpened():
            raise RuntimeError(f"Impossible d'ouvrir la source caméra: {src}")
        return cap, False

    path = Path(src_str).expanduser()
    if path.exists() and path.is_file() and path.suffix.lower() in IMAGE_EXTENSIONS:
        img = cv2.imread(str(path))
        if img is None:
            raise RuntimeError(f"Impossible de lire l'image: {path}")
        return img, True

    probe = str(path if path.exists() else src_str)
    cap = cv2.VideoCapture(probe)
    if not cap.isOpened():
        raise RuntimeError(f"Impossible d'ouvrir la source: {src}")
    return cap, False


def main():
    args = parse_args()

    # Prépare le modèle (TFLite ou Ultralytics .pt)
    detector = create_detector(weights=args.weights, imgsz=args.img, conf=args.conf, iou=args.iou, threads=1, device=(args.device or None))
    if args.debug_raw and hasattr(detector, 'debug_raw'):
        detector.debug_raw = True

    # Ouvre la source
    source_obj, is_image = open_source(args.source)

    class_filter = parse_class_filter(args.classes)

    window_name = 'TFLite – Detection panneaux'

    if is_image:
        frame = source_obj
        dets = detector.infer(frame, class_filter=class_filter)
        if args.print_dets:
            if dets:
                formatted = [
                    f"Obstacle {CLASS_NAMES.get(cid, f'class_{cid}')} conf={score:.2f} bbox=({x1},{y1},{x2},{y2})"
                    for x1, y1, x2, y2, score, cid in dets
                ]
                print(f"[DETECTIONS] {len(dets)} -> " + "; ".join(formatted))
            else:
                print("[DETECTIONS] Aucun objet détecté.")
        if args.no_show:
            print(f"[INFO] {len(dets)} détections sur l'image.")
        else:
            annotated = draw_detections(frame, dets, fps=None)
            cv2.imshow(window_name, annotated)
            cv2.waitKey(0)
            cv2.destroyAllWindows()
        return

    cap = source_obj
    paused = False
    t_prev = time.time()
    fps_ma = 0.0
    alpha = 0.9

    while True:
        if not paused:
            ok, frame = cap.read()
            if not ok:
                break

            dets = detector.infer(frame, class_filter=class_filter)
            if args.print_dets:
                if dets:
                    formatted = [
                        f"Obstacle {CLASS_NAMES.get(cid, f'class_{cid}')} conf={score:.2f} bbox=({x1},{y1},{x2},{y2})"
                        for x1, y1, x2, y2, score, cid in dets
                    ]
                    print(f"[DETECTIONS] {len(dets)} -> " + "; ".join(formatted))
                else:
                    print("[DETECTIONS] 0 objet détecté.")

            # FPS (moyenne mobile)
            now = time.time()
            dt = now - t_prev
            t_prev = now
            if dt > 0:
                inst = 1.0 / dt
                fps_ma = alpha * fps_ma + (1 - alpha) * inst
            annotated = draw_detections(frame, dets, fps=fps_ma)
        else:
            annotated = frame

        if args.no_show:
            print(f"[FRAME] dets={len(dets)} fps={fps_ma:0.2f}")
            key = None
        else:
            cv2.imshow(window_name, annotated)
            key = cv2.waitKey(1) & 0xFF
            if key == ord('q'):
                break
            if key == ord('p'):
                paused = not paused
        if args.max_fps and args.max_fps > 0:
            time.sleep(max(0.0, (1.0 / args.max_fps) - (time.time() - t_prev)))

    cap.release()
    cv2.destroyAllWindows()


if __name__ == '__main__':
    main()
