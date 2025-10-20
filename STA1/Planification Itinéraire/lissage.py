import json
import numpy as np

# === Paramètres ===
input_file = "Marvelmind_log_data.json"
output_file = "circuit_lisse.json"
window_size = 5  # doit être impair et > 1

with open(input_file, "r") as f:
    data = json.load(f)
# === 1. Charger les coordonnées depuis le JSON ===
points_raw = data["15"]
points_array = np.array([[p[1], p[2]] for p in points_raw])
points = points_array

x = np.array([pt["x"] for pt in data])
y = np.array([pt["y"] for pt in data])

# === 2. Fonction de lissage (moyenne glissante) ===
def smooth(values, window):
    if window < 3:
        return values
    pad = window // 2
    # On étend le signal au début et à la fin pour éviter les effets de bord
    padded = np.pad(values, (pad, pad), mode="edge")
    kernel = np.ones(window) / window
    return np.convolve(padded, kernel, mode="valid")

# === 3. Appliquer le lissage ===
x_smooth = smooth(x, window_size)
y_smooth = smooth(y, window_size)

# === 4. Sauvegarder le résultat ===
smoothed_data = [{"x": float(xn), "y": float(yn)} for xn, yn in zip(x_smooth, y_smooth)]
with open(output_file, "w") as f:
    json.dump(smoothed_data, f, indent=4)

print(f"✅ Tracé lissé sauvegardé dans {output_file}")