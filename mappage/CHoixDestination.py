#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import argparse
import json
from pathlib import Path
import socket

import numpy as np
import matplotlib.pyplot as plt

from scipy.signal import savgol_filter
HAS_SG = True

# ---------- Chargement des points depuis le JSON ----------
def load_points(json_path: str, key=None):
    """
    Attend un JSON de type:
    {
      "15": [[t, x, y, z], [t, x, y, z], ...],
      "16": ...
    }
    Retourne t, x, y pour la clé demandée (ou la première si key=None)
    """
    with open(json_path, "r", encoding="utf-8") as f:
        data = json.load(f)

    if key is None:
        # première clé du dict
        key = next(iter(data.keys()))

    # tolérance str/int
    if key not in data:
        alt = str(key) if not isinstance(key, str) else (int(key) if str(key).isdigit() else None)
        if alt is not None and alt in data:
            key = alt
        else:
            raise KeyError(f"Clé {key} introuvable dans {json_path}")

    arr = np.array(data[key], dtype=float)  # [N, >=3]
    if arr.shape[1] < 3:
        raise ValueError("Chaque point doit au minimum contenir [t, x, y].")

    t = arr[:, 0]
    x = arr[:, 1]
    y = arr[:, 2]
    return t, x, y, key


# ---------- Lissage ----------
def smooth_curve(x, y, enable=True):
    if not enable or not HAS_SG:
        return x, y

    n = len(x)
    if n < 7:
        return x, y
    # fenêtre impaire <= min(51, n)
    win = min(51, n if n % 2 == 1 else n - 1)
    if win < 5:
        win = 5 if 5 % 2 == 1 else 7
    poly = 3 if win >= 7 else 2

    xs = savgol_filter(x, win, poly)
    ys = savgol_filter(y, win, poly)
    return xs, ys


# ---------- Offsets gauche/droite ----------
def compute_offsets(x, y, offset_m):
    """
    Calcule des courbes offset tangentielles:
    Pour chaque point, on prend le vecteur tangent (dx,dy) puis le normal gauche = (-dy, dx) (normé).
    Gauche  = center + offset * normal_gauche
    Droite  = center - offset * normal_gauche
    """
    x = np.asarray(x, dtype=float)
    y = np.asarray(y, dtype=float)

    # dérivées centrales simples (gradient)
    dx = np.gradient(x)
    dy = np.gradient(y)

    # norme du vecteur tangent
    norm = np.hypot(dx, dy)
    # éviter division par zéro
    norm[norm == 0] = 1.0

    # normal gauche unitaire
    nx = -dy / norm
    ny =  dx / norm

    xL = x + offset_m * nx
    yL = y + offset_m * ny
    xR = x - offset_m * nx
    yR = y - offset_m * ny

    return (xL, yL), (xR, yR)


# ---------- UDP helpers ----------
def parse_hostport(value: str):
    host, sep, port = value.rpartition(":")
    if not sep:
        raise argparse.ArgumentTypeError("Format attendu HOST:PORT")
    try:
        return host, int(port)
    except ValueError:
        raise argparse.ArgumentTypeError("PORT doit être un entier")


def send_udp_xy(host: str, port: int, x: float, y: float):
    payload = {"x": float(x), "y": float(y)}
    data = json.dumps(payload).encode("utf-8")
    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as s:
        s.sendto(data, (host, port))

# ---------- Distance point -> polyligne (route centrale) ----------
def nearest_point_on_polyline(xs: np.ndarray, ys: np.ndarray, px: float, py: float):
    """
    Retourne (dist_min, sx, sy) où (sx,sy) est la projection du point P(px,py)
    sur la polyligne définie par (xs,ys). dist_min en mètres.
    """
    x = np.asarray(xs, dtype=float)
    y = np.asarray(ys, dtype=float)
    if len(x) < 2:
        return float('inf'), float(px), float(py)
    Ax = x[:-1]; Ay = y[:-1]
    Bx = x[1:];  By = y[1:]
    ABx = Bx - Ax; ABy = By - Ay
    APx = px - Ax; APy = py - Ay
    denom = ABx*ABx + ABy*ABy
    denom = np.where(denom == 0.0, 1.0, denom)
    t = (APx*ABx + APy*ABy) / denom
    t = np.clip(t, 0.0, 1.0)
    Sx = Ax + t*ABx
    Sy = Ay + t*ABy
    dx = px - Sx
    dy = py - Sy
    d2 = dx*dx + dy*dy
    idx = int(np.argmin(d2))
    return float(np.sqrt(d2[idx])), float(Sx[idx]), float(Sy[idx])



# ---------- Programme principal ----------
def main():
    ap = argparse.ArgumentParser(
        description="Trace route + courbes offset gauche/droite et envoie en UDP les coordonnées cliquées."
    )
    ap.add_argument(
        "--json",
        default="Marvelmind_log_data.json",
        help="Chemin du JSON d'entrée (défaut: mappage/Marvelmind_log_data.json)"
    )
    ap.add_argument("--key", help="Clé/ID à tracer (ex: 15). Par défaut: première clé du JSON.")
    ap.add_argument("--offset", type=float, default=0.05, help="Décalage (m) gauche/droite. Défaut: 0.05")
    ap.add_argument("--no-smooth", action="store_true", help="Désactive le lissage.")
    ap.add_argument(
        "--udp",
        type=parse_hostport,
        default=("127.0.0.1", 5005),
        help="Destination UDP HOST:PORT pour les clics (défaut: 127.0.0.1:5005)"
    )
    args = ap.parse_args()

    # 1) Charger les données
    t, x, y, used_key = load_points(args.json, args.key)

    # 2) Lissage (optionnel)
    xs, ys = smooth_curve(x, y, enable=(not args.no_smooth))

    # 3) Offsets
    (xL, yL), (xR, yR) = compute_offsets(xs, ys, args.offset)

    # 4) Plot
    fig, ax = plt.subplots(figsize=(7, 7))
    #ax.plot(xs, ys, "-", lw=1.2, label="Trajectoire", alpha=0.8)
    ax.plot(xL, yL, "--", lw=1.2, label=f"Gauche -{args.offset:.2f} m")
    ax.plot(xR, yR, "--", lw=1.2, label=f"Droite +{args.offset:.2f} m")

    ax.set_aspect("equal", adjustable="box")
    ax.grid(True, linestyle="--", linewidth=0.5)
    ax.set_xlabel("x (m)")
    ax.set_ylabel("y (m)")
    ax.set_title(f"Route + offsets (clé={used_key})")
    ax.legend()

    # 5) Clic -> envoi UDP des coordonnées cliquées
    host, port = args.udp
    marker = [None]

    def on_click(event):
        if event.inaxes != ax or event.xdata is None or event.ydata is None:
            return
        xc, yc = float(event.xdata), float(event.ydata)
        # envoi UDP
        try:
            send_udp_xy(host, port, xc, yc)
            print(f"[UDP] -> {host}:{port}  payload={{'x': {xc:.6f}, 'y': {yc:.6f}}}")
        except Exception as e:
            print(f"[UDP] échec: {e}")

        # feedback visuel: petit marqueur sur le clic
        if marker[0] is not None:
            marker[0].remove()
            marker[0] = None

        # Calculer la distance du clic à la route centrale et classifier avec le seuil = offset
        dist_min, sx, sy = nearest_point_on_polyline(xs, ys, xc, yc)
        on_route = dist_min <= args.offset
        status = "Point sur la route ✅" if on_route else "Point mal placé ❌"
        color = 'green' if on_route else 'red'

        marker[0] = ax.scatter([xc], [yc], s=70, facecolors='none', edgecolors=color, linewidths=1.5)

        # Afficher les coordonnées + statut en bas du graphique
        text_str = f"Point sélectionné : x = {xc:.3f} m, y = {yc:.3f} m — {status} "
        if hasattr(on_click, 'coord_text') and on_click.coord_text is not None:
            on_click.coord_text.set_text(text_str)
            on_click.coord_text.set_color(color)
        else:
            on_click.coord_text = fig.text(0.02, 0.02, text_str, fontsize=12, color=color)

        fig.canvas.draw_idle()


    fig.canvas.mpl_connect("button_press_event", on_click)
    print(f"Mode clic actif: coord. cliquées envoyées en UDP -> {host}:{port}")
    plt.show()


if __name__ == "__main__":
    main()