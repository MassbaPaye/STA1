import numpy as np
from PIL import Image
import matplotlib.pyplot as plt
import time

def color_probability_plateau(image_np, ref_color, d0=10, sigma=20, law="inv_quad"):
    """
    image_np : (H,W,3) RGB
    law : "quadratic" ou "inv_quad"
    """
    img = image_np.astype(np.float32)
    ref = np.array(ref_color, dtype=np.float32).reshape(1,1,3)
    diff = img - ref
    d = np.sqrt(np.sum(diff**2, axis=2))

    if law == "quadratic":
        p = np.ones_like(d)
        mask = d > d0
        p[mask] = 1 - ((d[mask] - d0)/sigma)**2
        p = np.clip(p, 0, 1)
    elif law == "inv_quad":
        p = np.ones_like(d)
        mask = d > d0
        p[mask] = 1 / (1 + ((d[mask]-d0)/sigma)**2)
    else:
        raise ValueError("law doit être 'quadratic' ou 'inv_quad'")
    return p

def show_images_in_batches(image_paths, ref_color=(200,30,30), d0=10, sigma=20, law="inv_quad", batch_size=3):
    """
    Affiche les images par lots (batch_size) avec original + filtrée
    """
    n = len(image_paths)
    for i in range(0, n, batch_size):
        batch = image_paths[i:i+batch_size]
        fig, axs = plt.subplots(len(batch), 2, figsize=(6,3*len(batch)))

        if len(batch) == 1:
            axs = [axs]

        for j, path in enumerate(batch):
            try:
                img = np.array(Image.open(path).convert("RGB"))
            except:
                print(f"[ERREUR] Impossible d'ouvrir {path}")
                continue

            t0 = time.time()
            prob = color_probability_plateau(img, ref_color, d0, sigma, law=law)
            elapsed = time.time() - t0

            axs[j][0].imshow(img)
            axs[j][0].set_title(f"{path} - Originale")
            axs[j][0].axis("off")

            axs[j][1].imshow(prob, cmap="gray", vmin=0, vmax=1)
            axs[j][1].set_title(f"Filtré rouge ({elapsed*1000:.1f} ms)")
            axs[j][1].axis("off")

        plt.suptitle(f"Détection rouge - loi: {law}, d0={d0}, sigma={sigma}", fontsize=14)
        plt.tight_layout()
        plt.show()

if __name__ == "__main__":
    images = [f"panneau{i}.jpg" for i in range(1,10)] +['panneau8.png']
    show_images_in_batches(images, ref_color=(190,20,20), d0=50, sigma=50, law="quadratic", batch_size=3)
