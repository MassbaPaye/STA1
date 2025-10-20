import cv2
import numpy as np
import time
from math import atan2, degrees

# --- CONFIG ---
INPUT_PATH = "map_centrale.png"
OUTPUT_PATH = "map_graph.png"
VISITED_RADIUS = 10           # rayon autour d'un pixel pour marquer visité
MAX_STEP = 60                # distance max pour chercher le prochain pixel
MAX_ANGLE_DEG = 50           # courbure max (en degrés)
LOG_INTERVAL = 1000          # log tous les N pixels visités
LINE_THICKNESS = 2           # largeur du segment pour le rendu

def log(msg):
    print(f"[{time.strftime('%H:%M:%S')}] {msg}")

def angle_between(p1, p2, p3):
    """Angle en degrés entre segments p1->p2 et p2->p3"""
    a, b, c = np.array(p1), np.array(p2), np.array(p3)
    v1, v2 = b - a, c - b
    if np.linalg.norm(v1)==0 or np.linalg.norm(v2)==0:
        return 0
    cos_theta = np.clip(np.dot(v1, v2)/(np.linalg.norm(v1)*np.linalg.norm(v2)), -1.0, 1.0)
    return degrees(np.arccos(cos_theta))

def main():
    log("Chargement de l'image...")
    img_bgr = cv2.imread(INPUT_PATH)
    if img_bgr is None:
        raise FileNotFoundError(INPUT_PATH)
    h, w, _ = img_bgr.shape

    # --- Masque rouge pur avec tolérance ---
    tol = 5
    red = img_bgr[:,:,2]
    green = img_bgr[:,:,1]
    blue = img_bgr[:,:,0]
    mask = (red >= 255-tol) & (green <= tol) & (blue <= tol)
    total_pixels = np.sum(mask)
    log(f"{total_pixels} pixels rouges détectés.")

    visited = np.zeros((h, w), dtype=np.uint8)
    paths = []
    pixels_visited_count = 0

    while pixels_visited_count < total_pixels:
        # Pixel de départ non visité
        ys, xs = np.where((mask==1) & (visited==0))
        if len(xs)==0:
            break
        start = (xs[0], ys[0])
        path = [start]

        # Marquer cercle autour du départ
        y0, x0 = start[1], start[0]
        y_min, y_max = max(0,y0-VISITED_RADIUS), min(h,y0+VISITED_RADIUS+1)
        x_min, x_max = max(0,x0-VISITED_RADIUS), min(w,x0+VISITED_RADIUS+1)
        newly_visited = np.sum((visited[y_min:y_max, x_min:x_max]==0))
        visited[y_min:y_max, x_min:x_max] = 1
        pixels_visited_count += newly_visited

        current = start
        prev = None

        while True:
            # Pixels candidats : non visités dans MAX_STEP autour
            ys_c, xs_c = np.where((mask==1) & (visited==0))
            if len(xs_c)==0:
                break
            dists = np.hypot(xs_c - current[0], ys_c - current[1])
            mask_d = dists <= MAX_STEP
            if not np.any(mask_d):
                break
            candidates = list(zip(xs_c[mask_d], ys_c[mask_d]))
            candidates.sort(key=lambda p: np.hypot(p[0]-current[0], p[1]-current[1]))

            # Chercher pixel valide selon angle
            found = False
            for cand in candidates:
                if prev is None or angle_between(prev, current, cand) <= MAX_ANGLE_DEG:
                    next_pixel = cand
                    found = True
                    break
            if not found:
                break

            # Ajouter au chemin et marquer cercle visité
            path.append(next_pixel)
            y0, x0 = next_pixel[1], next_pixel[0]
            y_min, y_max = max(0,y0-VISITED_RADIUS), min(h,y0+VISITED_RADIUS+1)
            x_min, x_max = max(0,x0-VISITED_RADIUS), min(w,x0+VISITED_RADIUS+1)
            newly_visited = np.sum((visited[y_min:y_max, x_min:x_max]==0))
            visited[y_min:y_max, x_min:x_max] = 1
            pixels_visited_count += newly_visited

            if pixels_visited_count % LOG_INTERVAL < newly_visited:
                log(f"Pixels visités: {pixels_visited_count}/{total_pixels} "
                    f"({pixels_visited_count/total_pixels*100:.2f}%)")

            prev = current
            current = next_pixel

        paths.append(path)
        log(f"Chemin {len(paths)}: {len(path)} pixels")

    # --- Affichage résultat avec lignes ---
    log("Création image graphe...")
    img_rgb = img_bgr.copy()
    colors = [(255,0,0),(0,255,0),(255,165,0),(255,0,255),(0,255,255)]
    for i, path in enumerate(paths):
        c = colors[i%len(colors)]
        for j in range(1, len(path)):
            cv2.line(img_rgb, path[j-1], path[j], c, thickness=LINE_THICKNESS)

    cv2.imwrite(OUTPUT_PATH, img_rgb)
    log(f"Graphe sauvegardé : {OUTPUT_PATH}")
    log("Terminé.")

if __name__=="__main__":
    main()
