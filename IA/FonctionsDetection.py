import argparse
import cv2
import os
import glob
import time
import struct
import math
import json
import socket
from dataclasses import dataclass, asdict

# Ajouts pour la détection TFLite
from functools import lru_cache
from pathlib import Path
from typing import List, Dict, Tuple, Optional, Iterable
import numpy as np

from libcamera import Transform  # ajout à faire en haut du fichier

try:
    from picamera2 import Picamera2,Transform
    _PICAMERA2_IMPORT_ERROR = None
except Exception as _err:
    Picamera2 = None  # Picamera2 n'est disponible que sur Raspberry Pi / libcamera
    _PICAMERA2_IMPORT_ERROR = _err

try:
    from ultralytics import YOLO as UltralyticsYOLO
    _ULTRALYTICS_IMPORT_ERROR = None
except Exception as _err:
    UltralyticsYOLO = None
    _ULTRALYTICS_IMPORT_ERROR = _err

# Modèle TFLite exporté localement (chemin par défaut vers best_float32.tflite)
DEFAULT_TFLITE_WEIGHTS = (
    Path(__file__).resolve().parent.parent / "models" / "best_saved_model" / "best_float32.tflite"
)

CLASS_NAMES = {
    0: "OBSTACLE_VOITURE",
    1: "PANNEAU_LIMITATION_30",
    2: "PANNEAU_BARRIERE",
    3: "PANNEAU_CEDER_PASSAGE",
    4: "PANNEAU_FIN30",
    5: "PANNEAU_INTERSECTION",
    6: "PANNEAU_PARKING",
    7: "PANNEAU_SENS_UNIQUE",
    8: "PONT",
}

DEFAULT_PHOTO_DIR = Path(__file__).resolve().parent / "Photos"
DEFAULT_PHOTO_FILENAME = "latest.jpg"

@dataclass
class Point:
    x: float
    y: float
    z: float = 0.0
    theta: float = 0.0


@dataclass
class Obstacle:
    label: str
    point_left: Point
    point_right: Point
    confidence: float
    class_id: int

CAMERA_HORIZONTAL_FOV_DEG = 70.0
# Vertical FOV optionally used for future extensions
CAMERA_VERTICAL_FOV_DEG = 55.0


# ------------------------- IMX500 – Acquisition brute -------------------------

IMX500_RES_PRESETS: Dict[str, Tuple[int, int]] = {
    "12mp": (4056, 3040),
    "2mp": (2028, 1520),
    "1080p": (1920, 1080),
    "720p": (1280, 720),
}

IMX500_DEFAULT_RAW_FORMAT = "SBGGR10"  # Bayer 10 bits typique des capteurs Sony


def _resolve_imx500_resolution(resolution: str | Tuple[int, int]) -> Tuple[int, int]:
    """
    Convertit une chaîne de preset ('12mp', '2028x1520', …) ou un tuple en (width, height).
    """
    if isinstance(resolution, tuple):
        return int(resolution[0]), int(resolution[1])
    res_lower = str(resolution).lower()
    if res_lower in IMX500_RES_PRESETS:
        return IMX500_RES_PRESETS[res_lower]
    if "x" in res_lower:
        w, h = res_lower.replace(" ", "").split("x", 1)
        return int(w), int(h)
    raise ValueError(f"Résolution IMX500 inconnue: {resolution}")

def imx500_open_camera(
    resolution: str | Tuple[int, int] = "2mp",
    raw_format: str = IMX500_DEFAULT_RAW_FORMAT,
    warmup: float = 1.0,
    controls: Optional[Dict[str, object]] = None,
) -> Tuple[Picamera2, Dict[str, object]]:
    """
    Initialise Picamera2 pour accéder à l'IMX500 et renvoie (picam2, info).

    - resolution: preset ou tuple (w, h)
    - raw_format: format du flux brut (ex: 'SBGGR10', 'SRGGB12')
    - warmup: délai en secondes pour stabiliser l'exposition
    - controls: dictionnaire optionnel de contrôles (AwbMode, ExposureTime, AnalogueGain…)

    La fonction lève RuntimeError si Picamera2 n'est pas présent (par exemple sur macOS).
    """
    if Picamera2 is None:
        raise RuntimeError(
            "Picamera2 n'est pas disponible sur cette plateforme. "
            "Installe 'python3-picamera2' sur Raspberry Pi pour accéder à l'IMX500."
        )

    size = _resolve_imx500_resolution(resolution)
    picam2 = Picamera2()

    raw_cfg = picam2.create_still_configuration(
        raw={"format": raw_format, "size": size},
        buffer_count=3,
    )
    picam2.configure(raw_cfg)
    picam2.start()

    if warmup > 0:
        time.sleep(warmup)

    if controls:
        picam2.set_controls(controls)
        # petit délai pour appliquer les contrôles (focus, expo…)
        time.sleep(0.2)

    info = {
        "resolution": size,
        "raw_format": raw_format,
        "controls": controls or {},
    }
    return picam2, info

def imx500_capture_raw_array(
    picam2: Picamera2,
    stream: str = "raw",
    bit_depth: int = 10,
    target_bit_depth: Optional[int] = None,
    subtract_black_level: bool = False,
    black_level: int = 64,
    normalize: bool = False,
) -> np.ndarray:
    """
    Capture un tableau brut (numpy) depuis l'IMX500.

    - stream: nom du flux Picamera2 ('raw' pour la capture brute, 'main' pour RGB)
    - bit_depth: profondeur de bits (10 bits par défaut)
    - target_bit_depth: profondeur de sortie souhaitée (ex. 16 pour obtenir un container 16 bits)
    - subtract_black_level: retire le niveau noir (utile pour du RAW 10 bits)
    - black_level: valeur soustraite si subtract_black_level=True
    - normalize: si True, renvoie un float32 normalisé [0,1]
    """
    if Picamera2 is None:
        raise RuntimeError("Picamera2 n'est pas disponible sur cette plateforme.")
    raw = picam2.capture_array(stream)
    if raw is None:
        raise RuntimeError("Capture RAW IMX500 vide.")

    # Picamera2 fournit déjà un np.ndarray; on vérifie le type
    arr = np.asarray(raw)

    # Soustraction éventuelle du niveau noir, dans l'échelle d'origine
    if subtract_black_level:
        arr = arr.astype(np.int32) - int(black_level)
        arr = np.clip(arr, 0, (1 << bit_depth) - 1).astype(np.uint16)

    # Conversion vers une profondeur cible (ex. 10 bits -> 16 bits)
    if target_bit_depth is not None and target_bit_depth != bit_depth:
        if not np.issubdtype(arr.dtype, np.integer):
            raise TypeError("La conversion de profondeur de bits exige un tableau entier (uint8/uint16).")
        arr = arr.astype(np.uint32)
        shift = int(target_bit_depth) - int(bit_depth)
        if shift > 0:
            arr = arr << shift
        elif shift < 0:
            arr = arr >> (-shift)
        max_val = (1 << target_bit_depth) - 1
        arr = np.clip(arr, 0, max_val)
        arr = arr.astype(np.uint16 if target_bit_depth > 8 else np.uint8)
        bit_depth = int(target_bit_depth)

    if normalize:
        arr = arr.astype(np.float32) / float((1 << bit_depth) - 1)

    return arr

def imx500_iter_raw_frames(
    picam2: Picamera2,
    *,
    limit: Optional[int] = None,
    sleep: float = 0.0,
) -> Iterable[np.ndarray]:
    """
    Générateur qui renvoie successivement des tableaux RAW provenant de l'IMX500.
    - limit: nombre maximum de frames (None → infini)
    - sleep: délai en secondes entre chaque capture (0 = enchaînement rapide)
    """
    if Picamera2 is None:
        raise RuntimeError("Picamera2 n'est pas disponible sur cette plateforme.")
    count = 0
    while limit is None or count < limit:
        yield imx500_capture_raw_array(picam2)
        count += 1
        if sleep > 0:
            time.sleep(sleep)

def imx500_close_camera(picam2: Optional[Picamera2]) -> None:
    """
    Arrête proprement la caméra IMX500 initialisée avec Picamera2.
    """
    if picam2 is None:
        return
    try:
        picam2.stop()
    finally:
        try:
            picam2.close()
        except Exception:
            pass

def imx500_load_raw_file(
    path: str | Path,
    resolution: str | Tuple[int, int],
    bit_depth: int = 10,
    dtype: np.dtype = np.uint16,
) -> np.ndarray:
    """
    Charge un fichier RAW binaire (par ex. `.raw` exporté depuis AITRIOS) en array numpy.
    - path: chemin vers le fichier brut
    - resolution: tuple (w,h) ou preset identique à imx500_open_camera
    - bit_depth: profondeur utile (10 bits par défaut)
    - dtype: type cible du tableau numpy (uint16 recommandé)
    """
    w, h = _resolve_imx500_resolution(resolution)
    path = Path(path)
    data = np.fromfile(path, dtype=dtype)
    expected = w * h
    if data.size != expected:
        raise ValueError(
            f"Le fichier RAW {path} ne correspond pas à {w}x{h} pixels "
            f"({expected} valeurs attendues, {data.size} trouvées)."
        )
    arr = data.reshape((h, w))
    if bit_depth < np.iinfo(dtype).bits:
        shift = np.iinfo(dtype).bits - bit_depth
        arr = (arr >> shift).astype(dtype)
    return arr

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


def capture_photo(
    output_dir: str | Path | None = None,
    filename: str = DEFAULT_PHOTO_FILENAME,
    resolution: str | Tuple[int, int] = "12mp",
    awb_mode: Optional[str] = None,
    exposure_mode: Optional[str] = None,
    warmup: float = 1.0,
) -> Tuple[np.ndarray, Path]:
    """
    Capture une image via Picamera2, l'enregistre et renvoie le frame (BGR).
    La photo remplace systématiquement le fichier `output_dir/filename`.
    """
    # Préparer le dossier de sortie
    output_path = Path(output_dir) if output_dir is not None else DEFAULT_PHOTO_DIR
    output_path = output_path.expanduser().resolve()
    output_path.mkdir(parents=True, exist_ok=True)
    photo_path = output_path / filename

    # Paramètres optionnels Picamera2
    controls: Dict[str, object] = {}
    if awb_mode:
        controls["AwbMode"] = awb_mode
    if exposure_mode:
        controls["ExposureMode"] = exposure_mode

    # Charger Picamera2 dynamiquement si besoin
    if Picamera2 is None:
        try:
            from picamera2 import Picamera2 as _Picamera2  # type: ignore
        except Exception as exc:
            msg = (
                "Picamera2 est indisponible. Vérifie l'installation de 'python3-picamera2' "
                "sur le Raspberry Pi (sudo apt install python3-picamera2) et exécute ce script sur la cible."
            )
            if _PICAMERA2_IMPORT_ERROR is not None:
                raise RuntimeError(msg) from _PICAMERA2_IMPORT_ERROR
            raise RuntimeError(msg) from exc
        Picamera2_local = _Picamera2
    else:
        Picamera2_local = Picamera2

    size = _resolve_imx500_resolution(resolution)
    picam2 = Picamera2_local()

    transform = Transform(hflip=True, vflip=True)  # adapte selon ce que tu observes
    config = picam2.create_still_configuration(
        main={"size": size, "format": "BGR888"},
        transform=transform,
        buffer_count=2,
    )
    picam2.configure(config)
    picam2.start()

    try:
        if warmup > 0:
            time.sleep(warmup)
        if controls:
            picam2.set_controls(controls)
            time.sleep(0.3)
        array = picam2.capture_array("main")
    finally:
        try:
            picam2.stop()
        finally:
            try:
                picam2.close()
            except Exception:
                pass

    if array is None:
        raise RuntimeError("Capture caméra vide.")

    # Conversion en BGR pour OpenCV / YOLO
    if array.ndim == 3 and array.shape[2] == 3:
        frame = cv2.cvtColor(array, cv2.COLOR_RGB2BGR)
    else:
        frame = np.asarray(array)

    cv2.imwrite(str(photo_path), frame)
    return frame, photo_path

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

# --- Transformation pixel -> coordonnées normalisées ---

def _pixel_to_signed_normalized(value: float, max_value: int) -> float:
    """
    Convertit un pixel en coordonnée normalisée [-1, 1] en prenant le centre
    de l'image comme origine (0). -1 = bord gauche/haut, +1 = bord droit/bas.
    """
    if max_value <= 1:
        return 0.0
    return ((float(value) / float(max_value - 1)) * 2.0) - 1.0


def _bbox_to_coordinate_points(
    bbox: Tuple[int, int, int, int],
    image_shape: Tuple[int, int, int],
) -> Tuple[Point, Point]:
    """
    Transforme une bounding box (en pixels) en deux points normalisés.
    Les coordonnées sont exprimées dans [-1, 1] pour x et y :
      - x = -1 → bord gauche, +1 → bord droit
      - y = -1 → haut, +1 → bas
    On ajoute également une estimation simple de la profondeur (z) et de l'angle horizontal (theta).
    """
    h, w = image_shape[:2]
    x1, y1, x2, y2 = bbox

    # Clamp pour rester dans l'image
    x1 = float(min(max(x1, 0), w - 1))
    x2 = float(min(max(x2, 0), w - 1))
    y1 = float(min(max(y1, 0), h - 1))
    y2 = float(min(max(y2, 0), h - 1))

    bottom_y = y2
    center_x = (x1 + x2) / 2.0
    center_y = (y1 + y2) / 2.0

    # Coordonnées normalisées   # question Victor normalement on devrait pas mettre y la distance qu'on calcul avec notre modèle expérimental, et en X la position latérale par rapport à la voiture avec Y*tan(theta-droite),Y*tan(theta gauche)
    left_point = Point(
        x=_pixel_to_signed_normalized(x1, w),
        y=_pixel_to_signed_normalized(bottom_y, h),
    )
    right_point = Point(
        x=_pixel_to_signed_normalized(x2, w),
        y=_pixel_to_signed_normalized(bottom_y, h),
    )

    # 4) "Profondeur relative" = largeur apparente de la bbox ppur les panneaux qu'on va stocker dans z 
    #    soit en pixels, soit normalisée par la largeur image
    width_px = abs(x2 - x1)
    depth_rel = width_px / w  # valeur normalisée entre 0 et 1

    # Orientation : on projette le centre sur l'angle horizontal de la caméra
    hfov_rad = math.radians(CAMERA_HORIZONTAL_FOV_DEG)
    theta = _pixel_to_signed_normalized(center_x, w) * (hfov_rad / 2.0)

    left_point.z = depth_rel
    right_point.z = depth_rel
    left_point.theta = theta
    right_point.theta = theta

    return left_point, right_point


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
        out_shape = tuple(int(v) for v in self.out.get('shape', ()))
        # Déterminer la dimension des features (4 bbox + obj? + classes)
        dims = [d for d in out_shape if d not in (0, 1)]
        feature_candidates = [d for d in dims if d > 4]
        self.feature_dim = min(feature_candidates) if feature_candidates else (dims[0] if dims else 0)
        self.num_classes = max(0, self.feature_dim - 4)

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
        if out.ndim == 3:
            out = out.reshape(out.shape[0], -1)
        if out.ndim != 2:
            return []
        c1, c2 = out.shape
        # Les features (4 + classes) sont généralement la plus petite dimension > 4
        if c1 < c2 and c1 <= max(512, self.feature_dim or 0):
            out = out.T
        if out.shape[1] < 6:
            return []
        xywh = out[:, :4]
        cls_block = out[:, 4:]
        if cls_block.size == 0:
            return []
        # Déterminer si une colonne d'objectness est incluse
        est_num_classes = self.num_classes or cls_block.shape[1]
        if cls_block.shape[1] == est_num_classes + 1:
            obj = cls_block[:, :1]
            cls_scores = cls_block[:, 1:]
        else:
            obj = None
            cls_scores = cls_block
            if cls_scores.shape[1] != est_num_classes and est_num_classes:
                est_num_classes = cls_scores.shape[1]
        # Certaines exports TFLite renvoient des coordonnees normalisees [0,1]
        # Si toutes les valeurs sont <= 1.5, on remultiplie par la taille d'entree du modele
        if np.all(xywh <= 1.5):
            xywh[:, 0] *= meta.get('in_w', self.in_w)
            xywh[:, 1] *= meta.get('in_h', self.in_h)
            xywh[:, 2] *= meta.get('in_w', self.in_w)
            xywh[:, 3] *= meta.get('in_h', self.in_h)
        # Si export a déjà activations, on suppose [0,1]; sinon appliquer sigmoïde
        if cls_scores.max() > 1.0 or cls_scores.min() < 0.0:
            cls_scores = 1.0 / (1.0 + np.exp(-cls_scores))
        if obj is not None:
            if obj.max() > 1.0 or obj.min() < 0.0:
                obj = 1.0 / (1.0 + np.exp(-obj))
            cls_scores = cls_scores * obj
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
    if not weights.exists():
        raise FileNotFoundError(f"Poids TFLite introuvables: {weights}")
    if weights.suffix.lower() != ".tflite":
        raise ValueError(
            f"Le fichier de poids fourni n'est pas un modèle TFLite: {weights}. "
            "Spécifie un fichier .tflite via --weights."
        )
    return TFLiteYOLO(weights=weights, imgsz=imgsz, conf=conf, iou=iou, threads=threads)


@lru_cache(maxsize=2)
def _load_ultralytics_model_cached(weights_str: str, device: str):
    if UltralyticsYOLO is None:
        msg = (
            "Le support des modèles .pt nécessite le paquet 'ultralytics'. "
            "Installe-le avec 'pip install ultralytics torch torchvision'."
        )
        if _ULTRALYTICS_IMPORT_ERROR is not None:
            raise RuntimeError(msg) from _ULTRALYTICS_IMPORT_ERROR
        raise RuntimeError(msg)

    weights_path = Path(weights_str)
    if not weights_path.exists():
        raise FileNotFoundError(f"Poids Ultralytics introuvables: {weights_path}")

    model = UltralyticsYOLO(weights_path)
    device_norm = (device or "").strip().lower()
    if device_norm and device_norm not in {"auto", "default"}:
        model.to(device_norm)
    return model

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
) -> List[Obstacle]:
    if not isinstance(image_bgr, np.ndarray):
        raise TypeError("image_bgr doit être un np.ndarray (image OpenCV BGR)")
    
    weights_path = str(Path(weights))
    detector = _load_tflite_detector_cached(weights_path, int(imgsz), float(conf), float(iou), int(threads))

    class_filter = _normalize_classes_arg(classes)

    dets = detector.infer(image_bgr, class_filter=class_filter)

    image_shape = image_bgr.shape
    obstacles: List[Obstacle] = []
    for (x1, y1, x2, y2, score, cid) in dets:
        class_id = int(cid)
        label = CLASS_NAMES.get(class_id, f"class_{class_id}")
        point_left, point_right = _bbox_to_coordinate_points((x1, y1, x2, y2), image_shape)
        obstacles.append(
            Obstacle(
                label=label,
                point_left=point_left,
                point_right=point_right,
                confidence=float(score),
                class_id=class_id,
            )
        )
    return obstacles


def detect_objects_ultralytics_from_image(
    image_bgr: np.ndarray,
    weights: str | Path,
    imgsz: int = 640,
    conf: float = 0.25,
    iou: float = 0.45,
    device: str | None = "cpu",
    classes=None,
) -> List[Obstacle]:
    if not isinstance(image_bgr, np.ndarray):
        raise TypeError("image_bgr doit être un np.ndarray (image OpenCV BGR)")

    weights_path = Path(weights)
    class_filter = _normalize_classes_arg(classes)
    device_key = (device or "cpu").strip().lower() or "cpu"

    model = _load_ultralytics_model_cached(str(weights_path), device_key)

    predict_kwargs = {
        "imgsz": int(imgsz),
        "conf": float(conf),
        "iou": float(iou),
        "verbose": False,
    }
    if device and device.strip().lower() not in {"", "auto", "default"}:
        predict_kwargs["device"] = device

    results = model.predict(image_bgr, **predict_kwargs)
    if not results:
        return []

    result = results[0]
    if result.boxes is None or result.boxes.xyxy is None or len(result.boxes) == 0:
        return []

    xyxy = result.boxes.xyxy.cpu().numpy()
    scores = result.boxes.conf.cpu().numpy()
    class_ids = result.boxes.cls.cpu().numpy().astype(int)

    mask = np.ones(len(xyxy), dtype=bool)
    if class_filter is not None:
        class_filter = set(class_filter)
        mask &= np.array([cid in class_filter for cid in class_ids])

    if mask.size and not mask.all():
        xyxy = xyxy[mask]
        scores = scores[mask]
        class_ids = class_ids[mask]

    image_shape = image_bgr.shape
    obstacles: List[Obstacle] = []
    for (x1, y1, x2, y2), score, class_id in zip(xyxy, scores, class_ids):
        label = CLASS_NAMES.get(int(class_id), f"class_{int(class_id)}")
        point_left, point_right = _bbox_to_coordinate_points(
            (int(x1), int(y1), int(x2), int(y2)), image_shape
        )
        obstacles.append(
            Obstacle(
                label=label,
                point_left=point_left,
                point_right=point_right,
                confidence=float(score),
                class_id=int(class_id),
            )
        )
    obstacles.sort(key=lambda obs: obs.confidence, reverse=True)
    return obstacles


def detect_obstacles_from_image(
    image_bgr: np.ndarray,
    weights: str | Path,
    *,
    imgsz: int = 640,
    conf: float = 0.25,
    iou: float = 0.45,
    threads: int = 2,
    device: str | None = "cpu",
    classes=None,
) -> List[Obstacle]:
    weights_path = Path(weights)
    suffix = weights_path.suffix.lower()

    if suffix == ".tflite":
        return detect_objects_tflite_from_image(
            image_bgr,
            weights_path,
            imgsz=imgsz,
            conf=conf,
            iou=iou,
            threads=threads,
            classes=classes,
        )
    if suffix == ".pt":
        return detect_objects_ultralytics_from_image(
            image_bgr,
            weights_path,
            imgsz=imgsz,
            conf=conf,
            iou=iou,
            device=device,
            classes=classes,
        )
    raise ValueError(
        f"Extension de modèle non supportée: '{suffix}'. "
        "Utilise un fichier .tflite ou .pt via --weights."
    )


def _build_obstacle_payload(obstacles: List[Obstacle]) -> Dict[str, object]:
    obstacles_payload = []
    for obs in obstacles:
        obs_dict = asdict(obs)
        obstacles_payload.append(obs_dict)
    return {
        "timestamp": time.time(),
        "count": len(obstacles_payload),
        "obstacles": obstacles_payload,
    }


def save_obstacles_to_json(
    obstacles: List[Obstacle],
    output_path: str | Path,
    *,
    ensure_ascii: bool = True,
    indent: int | None = 2,
) -> None:
    """
    Sérialise les obstacles détectés (coordonnées normalisées gauche/droite, label, classe, confiance) dans un fichier JSON.
    """
    output_path = Path(output_path).expanduser()
    output_path.parent.mkdir(parents=True, exist_ok=True)
    payload = _build_obstacle_payload(obstacles)
    with output_path.open("w", encoding="utf-8") as out:
        json.dump(payload, out, ensure_ascii=ensure_ascii, indent=indent)


def send_obstacles_udp(
    obstacles: List[Obstacle],
    host: str,
    port: int,
    *,
    timeout: Optional[float] = None,
) -> int:
    """
    Envoie la liste des obstacles détectés sur un socket UDP sous forme d'un message JSON unique.
    Le message contient uniquement des coordonnées relatives (points gauche/droite normalisés), le label,
    la classe numérique et la confiance associée.

    Retourne le nombre d'octets envoyés. Lève RuntimeError en cas d'échec d'envoi.
    """
    if not host:
        raise ValueError("Le paramètre 'host' ne peut pas être vide.")

    payload = _build_obstacle_payload(obstacles)
    data = json.dumps(payload, ensure_ascii=True, separators=(",", ":")).encode("utf-8")

    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    try:
        if timeout is not None:
            sock.settimeout(timeout)
        sent = sock.sendto(data, (host, int(port)))
    except OSError as exc:
        raise RuntimeError(f"Echec d'envoi UDP vers {host}:{port}: {exc}") from exc
    finally:
        sock.close()
    return sent


def draw_detections_on_image(img: np.ndarray, obstacles: List[Obstacle]) -> None:
    for obs in obstacles:
        left = obs.point_left
        right = obs.point_right
        score = obs.confidence

        h, w = img.shape[:2]
        # Convertir [-1, 1] -> pixels
        def _denorm(pt: Point) -> Tuple[int, int]:
            x_px = int(((pt.x + 1.0) * 0.5) * (w - 1))
            y_px = int(((pt.y + 1.0) * 0.5) * (h - 1))
            return x_px, y_px

        x1, y1 = _denorm(left)
        x2, y2 = _denorm(right)

        # Dessiner la base de l'obstacle (segment entre les deux points)
        cv2.line(img, (x1, y1), (x2, y2), (0, 255, 0), 2)
        cv2.circle(img, (x1, y1), 4, (0, 255, 0), -1)
        cv2.circle(img, (x2, y2), 4, (0, 255, 0), -1)

        label_text = f"{obs.label} {score:.2f}"
        text_origin_x = min(x1, x2)
        text_origin_y = max(0, min(y1, y2) - 12)
        cv2.putText(img, label_text, (text_origin_x, text_origin_y), cv2.FONT_HERSHEY_SIMPLEX, 0.6, (0, 255, 0), 2)


def main():
    import argparse

    parser = argparse.ArgumentParser(description="Capture photo, détection d'obstacles et envoi UDP (modèles TFLite ou Ultralytics .pt).")
    parser.add_argument("--host", default="127.0.0.1", help="IP du destinataire UDP (contrôleur).")
    parser.add_argument("--port", type=int, default=5005, help="Port UDP du contrôleur.")
    parser.add_argument("--weights", default=str(DEFAULT_TFLITE_WEIGHTS), help="Chemin vers le modèle (.tflite ou .pt).")
    parser.add_argument("--imgsz", type=int, default=640, help="Taille d'entrée du modèle YOLO (pixels).")
    parser.add_argument("--conf", type=float, default=0.25, help="Seuil de confiance minimum.")
    parser.add_argument("--iou", type=float, default=0.45, help="Seuil IoU pour la NMS.")
    parser.add_argument("--threads", type=int, default=2, help="Nombre de threads TFLite (ignoré pour .pt).")
    parser.add_argument("--device", default="cpu", help="Périphérique Ultralytics (.pt) : cpu, cuda:0, auto…")
    parser.add_argument("--photo-dir", default=None, help="Dossier où sauvegarder la photo (défaut: dossier Photos).")
    parser.add_argument("--photo-name", default=DEFAULT_PHOTO_FILENAME, help="Nom du fichier photo (défaut: latest.jpg).")
    parser.add_argument("--resolution", default="12mp", help="Résolution Picamera2 (ex: 12mp, 2mp, 1920x1080).")
    parser.add_argument("--awb", default=None, help="Mode balance des blancs (Picamera2).")
    parser.add_argument("--exposure", default=None, help="Mode d'exposition (Picamera2).")
    parser.add_argument("--warmup", type=float, default=1.0, help="Temps de préchauffage caméra (secondes).")
    parser.add_argument("--display", action="store_true", help="Affiche la photo annotée (requiert affichage).")
    args = parser.parse_args()

    try:
        frame, photo_path = capture_photo(
            output_dir=args.photo_dir,
            filename=args.photo_name,
            resolution=args.resolution,
            awb_mode=args.awb,
            exposure_mode=args.exposure,
            warmup=args.warmup,
        )
        print(f"Photo sauvegardée dans {photo_path}")
    except RuntimeError as exc:
        print(f"Erreur lors de la capture photo: {exc}")
        return

    try:
        obstacles = detect_obstacles_from_image(
            frame,
            Path(args.weights),
            imgsz=args.imgsz,
            conf=args.conf,
            iou=args.iou,
            threads=args.threads,
            device=args.device,
        )
    except Exception as exc:
        print(f"Erreur lors de la détection: {exc}")
        return

    print(f"{len(obstacles)} obstacle(s) détecté(s).")

    try:
        nbytes = send_obstacles_udp(obstacles, args.host, args.port)
        print(f"Obstacles envoyés ({nbytes} octets) à {args.host}:{args.port}")
    except RuntimeError as e:
        print(f"Erreur UDP: {e}")

    if args.display:
        annotated = frame.copy()
        draw_detections_on_image(annotated, obstacles)
        cv2.imshow("Detections", annotated)
        cv2.waitKey(0)
        cv2.destroyAllWindows()

if __name__ == "__main__":
    main()
