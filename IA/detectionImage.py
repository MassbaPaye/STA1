#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Détection de panneaux – Version YOLO uniquement

• Utilise la librairie `ultralytics` (YOLOv8/YOLOv11)
• Fonctionne sur ordinateur (CPU/GPU). Plus tard, tu pourras exporter en TFLite/ONNX pour le Pi/IMX500.

Installation (dans ton venv):
  pip install ultralytics opencv-python

Exemples:
  # Webcam avec modèle par défaut (yolov8n.pt)
  python3 detectionImage.py --source 0

  # Vidéo + poids spécifiques + seuil de confiance
  python3 detectionImage.py --source chemin/video.mp4 --weights yolov8n.pt --conf 0.35 --img 640

Touches:
  q -> quitter
  p -> pause/reprendre
"""

import argparse
from pathlib import Path
import time
import sys

import cv2
import numpy as np

from ultralytics import YOLO



def parse_args():
    ap = argparse.ArgumentParser(description="Détection YOLO (webcam/vidéo)")
    ap.add_argument('--source', default='0', help='Index caméra (0,1,...) ou chemin vidéo')
    ap.add_argument('--weights', type=Path, default=Path('yolov8n.pt'), help='Chemin vers les poids YOLO .pt')
    ap.add_argument('--conf', type=float, default=0.25, help='Seuil de confiance')
    ap.add_argument('--img', type=int, default=640, help='Taille d\'entrée (côté long)')
    ap.add_argument('--device', default='auto', help='cpu, 0, 0,1, etc. (auto par défaut)')
    ap.add_argument('--classes', type=str, default='', help="Filtre classes (ex: 'stop sign' ou '11' ou '11,12')")
    ap.add_argument('--max-fps', type=float, default=0.0, help='Limiter les FPS (0 = illimité)')
    return ap.parse_args()


def open_source(src):
    if str(src).isdigit() and len(str(src)) < 3:
        cap = cv2.VideoCapture(int(src))
    else:
        cap = cv2.VideoCapture(str(src))
    if not cap.isOpened():
        raise RuntimeError(f"Impossible d'ouvrir la source: {src}")
    return cap


def parse_class_filter(arg, names):
    """Retourne une liste d'IDs de classes à garder ou None pour ne pas filtrer.
       On accepte des noms (ex: "stop sign") ou des IDs (ex: "11,12")."""
    if not arg:
        return None
    arg = arg.strip()
    ids = []
    # Essaye IDs séparés par virgule
    if all(p.strip().isdigit() for p in arg.split(',')):
        return [int(p.strip()) for p in arg.split(',') if p.strip()]
    # Sinon mapper noms -> ids
    lower = {str(v).lower(): k for k, v in names.items()}
    for token in arg.split(','):
        t = token.strip().lower()
        if t in lower:
            ids.append(lower[t])
    return ids or None


def main():
    args = parse_args()

    # Charge le modèle
    model = YOLO(str(args.weights))  # télécharge auto si 'yolov8n.pt' absent

    # Ouvre la source
    cap = open_source(args.source)

    # Prépare le filtre de classes
    class_filter = parse_class_filter(args.classes, model.names)

    paused = False
    t_prev = time.time()
    fps_ma = 0.0
    alpha = 0.9

    while True:
        if not paused:
            ok, frame = cap.read()
            if not ok:
                break

            # Inférence YOLO (retourne une liste de Results)
            res = model.predict(
                source=frame,  # image numpy
                conf=args.conf,
                imgsz=args.img,
                device=args.device,
                verbose=False
            )[0]

            # Dessin personnalisé pour contrôler l'affichage
            boxes = res.boxes  # Boxes object
            annotated = frame.copy()

            if boxes is not None and len(boxes) > 0:
                xyxy = boxes.xyxy.cpu().numpy()
                confs = boxes.conf.cpu().numpy()
                clss  = boxes.cls.cpu().numpy().astype(int)

                for (x1, y1, x2, y2), sc, c in zip(xyxy, confs, clss):
                    if class_filter is not None and c not in class_filter:
                        continue
                    x1, y1, x2, y2 = map(int, [x1, y1, x2, y2])
                    cv2.rectangle(annotated, (x1, y1), (x2, y2), (0, 200, 255), 2)
                    name = model.names.get(c, str(c))
                    label = f"{name} {sc:.2f}"
                    (tw, th), _ = cv2.getTextSize(label, cv2.FONT_HERSHEY_SIMPLEX, 0.5, 1)
                    cv2.rectangle(annotated, (x1, y1 - th - 6), (x1 + tw + 6, y1), (0, 0, 0), -1)
                    cv2.putText(annotated, label, (x1 + 3, y1 - 4), cv2.FONT_HERSHEY_SIMPLEX, 0.5, (255,255,255), 1, cv2.LINE_AA)

            # FPS (moyenne mobile)
            now = time.time()
            dt = now - t_prev
            t_prev = now
            if dt > 0:
                inst = 1.0/dt
                fps_ma = alpha*fps_ma + (1-alpha)*inst
            cv2.putText(annotated, f"FPS: {fps_ma:5.1f}", (10, 22), cv2.FONT_HERSHEY_SIMPLEX, 0.6, (40,220,0), 2, cv2.LINE_AA)

        cv2.imshow('YOLO – Detection panneaux', annotated if not paused else frame)
        key = cv2.waitKey(1) & 0xFF
        if key == ord('q'):
            break
        if key == ord('p'):
            paused = not paused
        if args.max_fps and args.max_fps > 0:
            time.sleep(max(0.0, (1.0/args.max_fps) - (time.time() - t_prev)))

    cap.release()
    cv2.destroyAllWindows()


if __name__ == '__main__':
    main()
