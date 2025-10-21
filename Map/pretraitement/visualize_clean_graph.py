import csv
import math
import matplotlib.pyplot as plt

# === Fichiers nettoyés ===
NODES_FILE = "graph_data/nodes_clean.csv"
ARCS_FILE = "graph_data/arcs_clean.csv"


# === Chargement des fichiers ===
def load_nodes(path):
    """Charge les nœuds depuis le fichier CSV."""
    nodes = {}
    with open(path, newline='', encoding="utf-8") as f:
        reader = csv.DictReader(f)
        for row in reader:
            nodes[int(row["id"])] = (float(row["x"]), float(row["y"]))
    return nodes


def load_arcs(path):
    """Charge les arcs depuis le fichier CSV."""
    arcs = []
    with open(path, newline='', encoding="utf-8") as f:
        reader = csv.DictReader(f)
        for row in reader:
            u = int(row["u"])
            v = int(row["v"])
            radius = float("inf") if row["radius"] == "inf" else float(row["radius"])
            length = float(row["length"])
            cx = float(row["cx"]) if row["cx"] else None
            cy = float(row["cy"]) if row["cy"] else None
            arcs.append((u, v, radius, length, cx, cy))
    return arcs


# === Dessin d’un arc ou segment ===
def draw_arc(ax, p1, p2, radius, cx=None, cy=None, **kwargs):
    """Dessine un segment ou un arc circulaire."""
    x1, y1 = p1
    x2, y2 = p2

    if math.isinf(radius) or radius == 0 or cx is None or cy is None:
        # Segment droit
        ax.plot([x1, x2], [y1, y2], **kwargs)
        return

    # Angles relatifs
    ang1 = math.atan2(y1 - cy, x1 - cx)
    ang2 = math.atan2(y2 - cy, x2 - cx)
    dtheta = ang2 - ang1

    # Ajuster le sens selon le signe du rayon
    if radius > 0 and dtheta < 0:
        dtheta += 2 * math.pi
    elif radius < 0 and dtheta > 0:
        dtheta -= 2 * math.pi

    # Points de l’arc
    num_points = 80
    angles = [ang1 + t * dtheta / (num_points - 1) for t in range(num_points)]
    xs = [cx + abs(radius) * math.cos(a) for a in angles]
    ys = [cy + abs(radius) * math.sin(a) for a in angles]

    ax.plot(xs, ys, **kwargs)


# === Visualisation du graphe ===
def visualize_graph(nodes_file=NODES_FILE, arcs_file=ARCS_FILE):
    """Affiche les nœuds et arcs d’un graphe depuis les fichiers CSV nettoyés."""
    nodes = load_nodes(nodes_file)
    arcs = load_arcs(arcs_file)

    fig, ax = plt.subplots(figsize=(8, 8))
    ax.set_aspect("equal", adjustable="datalim")
    ax.invert_yaxis()  # Repère Y inversé pour correspondre à Tkinter ou autres outils

    # --- Afficher les arcs ---
    for (u, v, r, length, cx, cy) in arcs:
        p1, p2 = nodes[u], nodes[v]
        draw_arc(ax, p1, p2, r, cx, cy, color="green" if not math.isinf(r) else "cyan", linewidth=2)

    # --- Afficher les nœuds ---
    xs, ys = zip(*[nodes[i] for i in nodes])
    ax.scatter(xs, ys, s=30, color="blue", zorder=5)

    # --- Annoter les nœuds ---
    for i, (x, y) in nodes.items():
        ax.text(x + 5, y + 5, str(i), color="yellow", fontsize=8, weight="bold",
                bbox=dict(facecolor="black", alpha=0.4, pad=1))

    ax.set_title("Visualisation du graphe nettoyé")
    plt.show()


# === Point d’entrée ===
if __name__ == "__main__":
    visualize_graph()
