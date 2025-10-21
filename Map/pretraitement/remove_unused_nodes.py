import csv
import math
import os

# === Chemins des fichiers ===
NODES_FILE = "graph_data/nodes.csv"
ARCS_FILE = "graph_data/arcs.csv"

OUTPUT_NODES_FILE = "graph_data/nodes_clean.csv"
OUTPUT_ARCS_FILE = "graph_data/arcs_clean.csv"


# === Chargement des fichiers ===
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
            u = int(row["u"])
            v = int(row["v"])
            radius = float("inf") if row["radius"] == "inf" else float(row["radius"])
            arcs.append((u, v, radius))
    return arcs


# === Recalcule du centre et de la longueur ===
def compute_arc_geometry(p1, p2, radius):
    """Renvoie (length, cx, cy) pour un arc reliant p1 à p2 avec rayon donné."""
    x1, y1 = p1
    x2, y2 = p2

    if math.isinf(radius):
        # Segment droit
        length = math.hypot(x2 - x1, y2 - y1)
        return length, None, None

    dx, dy = x2 - x1, y2 - y1
    d = math.hypot(dx, dy)

    if d > 2 * abs(radius):
        # Impossible de relier les deux points avec ce rayon
        length = d
        return length, None, None

    # Milieu du segment
    mx, my = (x1 + x2) / 2, (y1 + y2) / 2

    # Distance du milieu au centre
    h = math.sqrt(radius**2 - (d / 2)**2)

    # Vecteur perpendiculaire unitaire
    nx, ny = -dy / d, dx / d

    # Choisir le côté selon le signe du rayon
    cx = mx + (h if radius > 0 else -h) * nx
    cy = my + (h if radius > 0 else -h) * ny

    # Angles
    ang1 = math.atan2(y1 - cy, x1 - cx)
    ang2 = math.atan2(y2 - cy, x2 - cx)

    # Ajustement du sens
    dtheta = ang2 - ang1
    if radius > 0 and dtheta < 0:
        dtheta += 2 * math.pi
    elif radius < 0 and dtheta > 0:
        dtheta -= 2 * math.pi

    # Longueur d’arc
    length = abs(radius * dtheta)
    return length, cx, cy


# === Traitement principal ===
def clean_graph(nodes_file=NODES_FILE, arcs_file=ARCS_FILE):
    nodes = load_nodes(nodes_file)
    arcs = load_arcs(arcs_file)

    # 1️⃣ Identifier les nœuds utilisés
    used_nodes = set()
    for (u, v, _) in arcs:
        used_nodes.add(u)
        used_nodes.add(v)

    # 2️⃣ Supprimer les nœuds inutilisés
    nodes = {i: nodes[i] for i in used_nodes if i in nodes}

    # 3️⃣ Re-indexer les nœuds à partir de 0
    id_map = {old_id: new_id for new_id, old_id in enumerate(sorted(nodes.keys()))}
    new_nodes = {id_map[i]: nodes[i] for i in nodes}

    # 4️⃣ Recalculer les arcs
    new_arcs = []
    for (u, v, radius) in arcs:
        if u not in id_map or v not in id_map:
            continue
        p1, p2 = new_nodes[id_map[u]], new_nodes[id_map[v]]
        length, cx, cy = compute_arc_geometry(p1, p2, radius)
        new_arcs.append({
            "u": id_map[u],
            "v": id_map[v],
            "radius": radius if not math.isinf(radius) else "inf",
            "length": length,
            "cx": "" if cx is None else cx,
            "cy": "" if cy is None else cy
        })

    # 5️⃣ Sauvegarde des résultats
    os.makedirs(os.path.dirname(OUTPUT_NODES_FILE), exist_ok=True)

    with open(OUTPUT_NODES_FILE, "w", newline="", encoding="utf-8") as f:
        writer = csv.DictWriter(f, fieldnames=["id", "x", "y"])
        writer.writeheader()
        for i, (x, y) in new_nodes.items():
            writer.writerow({"id": i, "x": x, "y": y})

    with open(OUTPUT_ARCS_FILE, "w", newline="", encoding="utf-8") as f:
        writer = csv.DictWriter(f, fieldnames=["u", "v", "radius", "length", "cx", "cy"])
        writer.writeheader()
        writer.writerows(new_arcs)

    print(f"✅ Nœuds nettoyés : {len(new_nodes)} sauvegardés dans {OUTPUT_NODES_FILE}")
    print(f"✅ Arcs recalculés : {len(new_arcs)} sauvegardés dans {OUTPUT_ARCS_FILE}")


# === Exécution directe ===
if __name__ == "__main__":
    clean_graph()
