
#Enregistre le code ci-dessous dans capture_photos.py sur le Pi.


#!/usr/bin/env python3
import argparse, time
from datetime import datetime
from pathlib import Path
from picamera2 import Picamera2

RES_PRESETS = {
    "2mp": (2028, 1520),     # ~3:2 à ~2MP (p30 en vidéo; pour photo c'est instantané)
    "12mp": (4056, 3040),    # pleine résolution capteur (p10 en vidéo)
}

def build_camera(res_key: str, awb: str, exposure: str):
    picam2 = Picamera2()

    # Résolution
    if res_key in RES_PRESETS:
        size = RES_PRESETS[res_key]
    else:
        # format "LxH", ex: "2560x1440"
        try:
            w, h = map(int, res_key.lower().split("x"))
            size = (w, h)
        except Exception:
            raise ValueError("Résolution invalide. Utilise '2mp', '12mp' ou 'LxH' (ex: 2560x1440).")

    # Configuration pour la photo (still)
    config = picam2.create_still_configuration(
        main={"size": size, "format": "RGB888"},
        buffer_count=2
    )
    picam2.configure(config)

    # Contrôles simples (optionnels)
    # AWB modes: 'auto','incandescent','tungsten','fluorescent','indoor','daylight','cloudy','custom'
    # exposure: 'normal' (auto) ou 'short'/'long' selon Picamera2
    controls = {}
    if awb:
        controls["AwbMode"] = awb
    if exposure:
        controls["ExposureMode"] = exposure

    picam2.start()
    time.sleep(1.0)  # petit délai pour stabiliser l'expo/awb

    if controls:
        picam2.set_controls(controls)
        time.sleep(0.3)

    return picam2, size

def save_capture(picam2, out_dir: Path, prefix: str, ext: str = "jpg"):
    out_dir.mkdir(parents=True, exist_ok=True)
    ts = datetime.now().strftime("%Y%m%d_%H%M%S_%f")
    path = out_dir / f"{prefix}_{ts}.{ext}"
    picam2.capture_file(str(path), name="main")  # JPEG par défaut si ext=jpg
    return path

def main():
    parser = argparse.ArgumentParser(description="Prendre des photos avec Picamera2 (IMX500/CSI).")
    parser.add_argument("--out", default="photos", help="Dossier de sortie (défaut: photos)")
    parser.add_argument("--prefix", default="img", help="Préfixe du nom de fichier (défaut: img)")
    parser.add_argument("--res", default="12mp",
                        help="Résolution: '2mp', '12mp' ou 'LxH' (ex: 2560x1440). Défaut: 12mp")
    parser.add_argument("--count", type=int, default=1, help="Nombre d’images à capturer (défaut: 1)")
    parser.add_argument("--interval", type=float, default=0.0,
                        help="Intervalle en secondes entre prises (si count>1). Défaut: 0")
    parser.add_argument("--awb", default=None,
                        help="Balance des blancs (ex: auto, daylight, cloudy...). Défaut: auto")
    parser.add_argument("--exposure", default=None,
                        help="Mode d’exposition (ex: normal). Laisse vide pour auto.")
    parser.add_argument("--ext", default="jpg", choices=["jpg","png","bmp","dng"],
                        help="Format de sauvegarde (défaut: jpg). 'dng' = RAW.")
    args = parser.parse_args()

    out_dir = Path(args.out)
    picam2, size = build_camera(args.res, args.awb, args.exposure)
    print(f"[INFO] Cam initialisée en {size[0]}x{size[1]} → dossier: {out_dir.resolve()}")

    try:
        for i in range(args.count):
            path = save_capture(picam2, out_dir, args.prefix, args.ext)
            print(f"[OK] Capture {i+1}/{args.count} → {path.name}")
            if i < args.count - 1 and args.interval > 0:
                time.sleep(args.interval)
    finally:
        picam2.stop()
        print("[INFO] Terminé.")

if __name__ == "__main__":
    main()
