import cv2
import numpy as np
import matplotlib.pyplot as plt
import time

INPUT_PATH = "map_reduced.png"
OUTPUT_PATH = "map_centrale.png"

def log(msg):
    print(f"[{time.strftime('%H:%M:%S')}] {msg}")

def calcul_ligne_centrale(image_path, dilation_size=3):
    """
    Génère une ligne centrale un peu plus épaisse.
    
    dilation_size : taille du kernel de dilation (plus grand = ligne plus épaisse)
    """
    log("Chargement de l’image...")
    img = cv2.imread(image_path, cv2.IMREAD_GRAYSCALE)
    if img is None:
        raise FileNotFoundError(f"Impossible de lire {image_path}")

    # Zones d'intérêt : gris (chaussée)
    mask_road = (img == 128).astype(np.uint8)
    mask_bord = (img == 0).astype(np.uint8)

    # Transformée de distance : distance de chaque pixel route au bord noir
    log("Calcul de la transformée de distance...")
    dist = cv2.distanceTransform(1 - mask_bord, cv2.DIST_L2, 3)
    dist *= mask_road  # on ne garde que la route

    # Extraction des maxima locaux (approximation des centres)
    log("Détection des maxima locaux...")
    kernel = np.ones((3, 3), np.uint8)
    local_max = cv2.dilate(dist, kernel) == dist
    ligne_centrale = np.logical_and(local_max, mask_road)
    ligne_centrale = ligne_centrale.astype(np.uint8) * 255

    # --- ÉPAISSIR LA LIGNE ---
    if dilation_size > 1:
        kernel_dilate = cv2.getStructuringElement(cv2.MORPH_ELLIPSE, (dilation_size, dilation_size))
        ligne_centrale = cv2.dilate(ligne_centrale, kernel_dilate)

    # Conversion en image couleur (BGR pour OpenCV)
    img_bgr = cv2.cvtColor(img, cv2.COLOR_GRAY2BGR)

    # Colorier la ligne centrale en rouge (BGR = 0,0,255)
    img_bgr[ligne_centrale > 0] = (0, 0, 255)

    # Sauvegarde du résultat
    cv2.imwrite(OUTPUT_PATH, img_bgr)
    log(f"Ligne centrale sauvegardée : {OUTPUT_PATH}")

    # Affichage avec matplotlib (converti en RGB pour affichage correct)
    img_rgb = cv2.cvtColor(img_bgr, cv2.COLOR_BGR2RGB)
    plt.figure(figsize=(12, 8))
    plt.imshow(img_rgb)
    plt.title("Ligne centrale (rouge) épaissie")
    plt.axis("off")
    plt.savefig("debug_ligne_centrale.png", dpi=150)
    plt.show()

def main():
    start = time.time()
    calcul_ligne_centrale(INPUT_PATH, dilation_size=5)  # ajuster la taille ici
    log(f"⏱️ Terminé en {time.time() - start:.1f}s")

if __name__ == "__main__":
    main()
