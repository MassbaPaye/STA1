import os
import pandas as pd
import cv2

# --- Dossier contenant les images ---
image_folder = "IA/Photos"
image_files = [f for f in os.listdir(image_folder) if f.lower().endswith(('.jpg', '.png'))]

clicks = []
current_image_index = 0
image = None

def show_coordinates(event, x, y, flags, param):
    global image
    if event == cv2.EVENT_LBUTTONDOWN:  # clic gauche
        filename = image_files[current_image_index]
        print(f"Pixel cliqué : x={x}, y={y} sur l'image {filename}")
        clicks.append({'image': filename, 'x': x, 'y': y})
        # (Optionnel) afficher un point rouge à l'endroit du clic
        cv2.circle(image, (x, y), 3, (0, 0, 255), -1)
        cv2.imshow("Image", image)

cv2.namedWindow("Image")
cv2.setMouseCallback("Image", show_coordinates)

print("Clique sur l'image (appuie sur 'q' pour passer à l'image suivante ou quitter)")

while current_image_index < len(image_files):
    image_path = os.path.join(image_folder, image_files[current_image_index])
    image = cv2.imread(image_path)

    if image is None:
        print(f"❌ Erreur : impossible de charger l'image {image_files[current_image_index]}.")
        current_image_index += 1
        continue

    cv2.imshow("Image", image)

    while True:
        key = cv2.waitKey(1) & 0xFF
        if key == ord('q'):
            current_image_index += 1
            break

cv2.destroyAllWindows()

df = pd.DataFrame(clicks)
print(df)
csv_path = os.path.join(image_folder, "click_coordinates.csv")
df.to_csv(csv_path, index=False)
print(f"Coordonnées enregistrées dans {csv_path}")