import os
import cv2
import numpy as np
import matplotlib.pyplot as plt
import cairosvg
import time

# --- CONFIGURATION ---
SVG_PATH = "map.svg"
PNG_PATH = "map.png"
OUTPUT_PATH = "map_reduced.png"

# --- COULEURS ---
COLOR_BLACK = (0, 0, 0)
COLOR_GRAY = (128, 128, 128)
BLOCK_SIZE = 8  # Taille de bloc (plus grand = réduction plus forte)

def log(msg):
    print(f"[{time.strftime('%H:%M:%S')}] {msg}")

def convert_svg_to_png(svg_path, png_path):
    """Convertit un SVG en PNG si le PNG n'existe pas déjà."""
    if not os.path.exists(png_path):
        log(f"Conversion {svg_path} → {png_path}")
        cairosvg.svg2png(url=svg_path, write_to=png_path)
    else:
        log(f"PNG déjà existant ({png_path})")

def filtre_routes(image_bgr):
    """Crée deux masques binaires : noir et gris."""
    image_rgb = cv2.cvtColor(image_bgr, cv2.COLOR_BGR2RGB)

    # Noir : #000000 ± tolérance
    lower_black = np.array([0, 0, 0])
    upper_black = np.array([40, 40, 40])
    mask_black = cv2.inRange(image_rgb, lower_black, upper_black)

    # Gris : #808080 ± tolérance
    lower_gray = np.array([100, 100, 100])
    upper_gray = np.array([160, 160, 160])
    mask_gray = cv2.inRange(image_rgb, lower_gray, upper_gray)

    return mask_black, mask_gray

def reduction_bloc(mask_black, mask_gray, n=4):
    """Réduction simple par blocs non superposés."""
    h, w = mask_black.shape
    new_h, new_w = h // n, w // n
    log(f"Réduction : {w}x{h} → {new_w}x{new_h}")

    reduced = np.full((new_h, new_w), 255, dtype=np.uint8)  # fond blanc

    for i in range(new_h):
        if i % 500 == 0:
            print(f"Progression : {i}/{new_h}")
        for j in range(new_w):
            y0, y1 = i * n, (i + 1) * n
            x0, x1 = j * n, (j + 1) * n

            block_black = mask_black[y0:y1, x0:x1]
            block_gray = mask_gray[y0:y1, x0:x1]

            if np.any(block_black > 0):
                reduced[i, j] = 0        # noir
            elif np.any(block_gray > 0):
                reduced[i, j] = 128      # gris

    return reduced

def afficher_et_sauver(mask_reduit):
    """Affiche et sauvegarde le résultat."""
    plt.figure(figsize=(12, 8))
    plt.imshow(mask_reduit, cmap="gray")
    plt.title("Carte réduite (préservation des routes)")
    plt.axis("off")
    plt.show()

    cv2.imwrite(OUTPUT_PATH, mask_reduit)
    log(f"Image sauvegardée : {OUTPUT_PATH}")

def main():
    start = time.time()

    convert_svg_to_png(SVG_PATH, PNG_PATH)

    log("Chargement de l’image...")
    image_bgr = cv2.imread(PNG_PATH)
    if image_bgr is None:
        log("❌ Impossible de lire l’image PNG.")
        return

    log("Filtrage des routes...")
    mask_black, mask_gray = filtre_routes(image_bgr)
    log(f"Pixels noirs : {np.count_nonzero(mask_black)} | gris : {np.count_nonzero(mask_gray)}")

    log("Réduction par blocs...")
    mask_reduit = reduction_bloc(mask_black, mask_gray, BLOCK_SIZE)

    log("Affichage et sauvegarde...")
    afficher_et_sauver(mask_reduit)

    log(f"⏱️ Terminé en {time.time() - start:.1f}s")

if __name__ == "__main__":
    main()
