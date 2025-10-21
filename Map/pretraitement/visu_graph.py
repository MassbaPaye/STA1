import csv
import math
import matplotlib.pyplot as plt

NODES_FILE = "graph_data/nodes.csv"
ARCS_FILE = "graph_data/arcs.csv"

def load_nodes(path):
    nodes = {}
    with open(path, newline='', encoding="utf-8") as f:
        reader = csv.DictReader(f)
        for row in reader:
            nodes[int(row["id"])] = (float(row["x"]), float(row["y"]))
    return nodes

def load_arcs(path):
    arcs = []
    with open(path, newline='', encoding="utf-8") as f:
        reader = csv.DictReader(f)
        for row in reader:
            print(row)
            u = int(row["u"])
            v = int(row["v"])
            radius = float("inf") if row["radius"] == "inf" else float(row["radius"])
            length = float("inf") if row["length"] == "" else float(row["length"])
            cx = float(row["cx"]) if row["cx"] else None
            cy = float(row["cy"]) if row["cy"] else None
            arcs.append((u, v, radius, length, cx, cy))
    return arcs

def draw_arc(ax, p1, p2, radius, **kwargs):
    """Dessine un segment ou un arc circulaire à partir des points et du rayon."""
    x1, y1 = p1
    x2, y2 = p2

    if math.isinf(radius) or radius == 0:
        # Segment droit
        ax.plot([x1, x2], [y1, y2], **kwargs)
        return

    # Distance entre les points
    dx = x2 - x1
    dy = y2 - y1
    d = math.hypot(dx, dy)

    if d > 2 * abs(radius):
        # Rayon trop petit pour relier les deux points
        ax.plot([x1, x2], [y1, y2], linestyle="--", color="red")
        return

    # Milieu du segment
    mx, my = (x1 + x2) / 2, (y1 + y2) / 2

    # Distance du milieu au centre
    h = math.sqrt(radius**2 - (d / 2)**2)

    # Vecteur perpendiculaire normalisé
    nx, ny = -dy / d, dx / d

    # Choisir le bon côté selon le signe du rayon
    cx = mx + (h if radius > 0 else -h) * nx
    cy = my + (h if radius > 0 else -h) * ny

    # Angles relatifs
    ang1 = math.atan2(y1 - cy, x1 - cx)
    ang2 = math.atan2(y2 - cy, x2 - cx)

    # Ajuster le sens de rotation selon le signe du rayon
    if radius > 0 and ang2 < ang1:
        ang2 += 2 * math.pi
    elif radius < 0 and ang2 > ang1:
        ang2 -= 2 * math.pi

    # Points de l'arc
    num_points = 80
    angles = [ang1 + t * (ang2 - ang1) / (num_points - 1) for t in range(num_points)]
    xs = [cx + abs(radius) * math.cos(a) for a in angles]
    ys = [cy + abs(radius) * math.sin(a) for a in angles]

    ax.plot(xs, ys, **kwargs)


def visualize_graph(nodes_file=NODES_FILE, arcs_file=ARCS_FILE):
    nodes = load_nodes(nodes_file)
    arcs = load_arcs(arcs_file)

    fig, ax = plt.subplots(figsize=(8, 8))
    ax.set_aspect("equal", adjustable="datalim")
    ax.invert_yaxis()  # car Tkinter utilise un repère inversé en Y

    # --- Afficher les arcs ---
    for (u, v, r, length, cx, cy) in arcs:
        p1, p2 = nodes[u], nodes[v]
        draw_arc(ax, p1, p2, r, color="green" if not math.isinf(r) else "cyan", linewidth=2)


    # --- Afficher les nœuds ---
    xs, ys = zip(*[nodes[i] for i in nodes])
    ax.scatter(xs, ys, s=30, color="blue", zorder=5)

    # --- Annoter les nœuds ---
    for i, (x, y) in nodes.items():
        ax.text(x+5, y+5, str(i), color="yellow", fontsize=8, weight="bold",
                bbox=dict(facecolor="black", alpha=0.4, pad=1))

    ax.set_title("Visualisation du graphe exporté")
    plt.show()

if __name__ == "__main__":
    visualize_graph()

