import csv
import math
import os

# --- Fichiers ---
NODES_FILE = "graph_data/nodes.csv"
ARCS_FILE = "graph_data/arcs.csv"

NODES_CLEAN_FILE = "graph_data/nodes_clean.csv"
ARCS_CLEAN_FILE = "graph_data/arcs_clean.csv"

ARCS_ORIENTED_FILE = "graph_data/arcs_oriented.csv"


# === Chargement ===
def load_nodes(path):
    nodes = {}
    with open(path, newline="", encoding="utf-8") as f:
        reader = csv.DictReader(f)
        for row in reader:
            nodes[int(row["id"])] = (float(row["x"]), float(row["y"]))
    return nodes


def load_arcs(path):
    arcs = []
    with open(path, newline="", encoding="utf-8") as f:
        reader = csv.DictReader(f)
        for row in reader:
            u = int(row["u"])
            v = int(row["v"])
            radius = float("inf") if row["radius"] == "inf" else float(row["radius"])
            arcs.append((u, v, radius))
    return arcs


# === Calcul longueur et centre ===
def compute_arc_geometry(p1, p2, radius):
    x1, y1 = p1
    x2, y2 = p2
    if math.isinf(radius):
        length = math.hypot(x2 - x1, y2 - y1)
        return length, None, None

    dx, dy = x2 - x1, y2 - y1
    d = math.hypot(dx, dy)
    if d > 2 * abs(radius):
        return d, None, None

    mx, my = (x1 + x2) / 2, (y1 + y2) / 2
    h = math.sqrt(radius ** 2 - (d / 2) ** 2)
    nx, ny = -dy / d, dx / d
    cx = mx + (h if radius > 0 else -h) * nx
    cy = my + (h if radius > 0 else -h) * ny

    ang1 = math.atan2(y1 - cy, x1 - cx)
    ang2 = math.atan2(y2 - cy, x2 - cx)
    dtheta = ang2 - ang1
    if radius > 0 and dtheta < 0:
        dtheta += 2 * math.pi
    elif radius < 0 and dtheta > 0:
        dtheta -= 2 * math.pi

    length = abs(radius * dtheta)
    return length, cx, cy


# === Étape 1 : Nettoyage initial ===
def clean_graph(nodes_file, arcs_file):
    nodes = load_nodes(nodes_file)
    arcs = load_arcs(arcs_file)

    # nœuds utilisés
    used_nodes = set()
    for u, v, _ in arcs:
        used_nodes.add(u)
        used_nodes.add(v)
    nodes = {i: nodes[i] for i in used_nodes if i in nodes}

    # recalcul des arcs (longueur + centre)
    clean_arcs = []
    for u, v, radius in arcs:
        if u not in nodes or v not in nodes:
            continue
        p1, p2 = nodes[u], nodes[v]
        length, cx, cy = compute_arc_geometry(p1, p2, radius)
        clean_arcs.append({
            "u": u,
            "v": v,
            "radius": radius if not math.isinf(radius) else "inf",
            "length": length,
            "cx": "" if cx is None else cx,
            "cy": "" if cy is None else cy
        })

    # sauvegarde fichiers clean
    os.makedirs(os.path.dirname(NODES_CLEAN_FILE), exist_ok=True)

    with open(NODES_CLEAN_FILE, "w", newline="", encoding="utf-8") as f:
        writer = csv.DictWriter(f, fieldnames=["id", "x", "y"])
        writer.writeheader()
        for i, (x, y) in nodes.items():
            writer.writerow({"id": i, "x": x, "y": y})

    with open(ARCS_CLEAN_FILE, "w", newline="", encoding="utf-8") as f:
        writer = csv.DictWriter(f, fieldnames=["u", "v", "radius", "length", "cx", "cy"])
        writer.writeheader()
        writer.writerows(clean_arcs)

    print(f"✅ Fichiers clean créés : {NODES_CLEAN_FILE}, {ARCS_CLEAN_FILE}")


# === Étape 2 : Orientation interactive ===
def orient_graph(clean_nodes_file, clean_arcs_file, oriented_file):
    nodes = load_nodes(clean_nodes_file)
    arcs = load_arcs(clean_arcs_file)

    os.makedirs(os.path.dirname(oriented_file), exist_ok=True)
    with open(oriented_file, "w", newline="", encoding="utf-8") as f:
        writer = csv.DictWriter(f, fieldnames=["u", "v", "radius", "length", "cx", "cy"])
        writer.writeheader()

        for u, v, radius in arcs:
            while True:
                print(f"Arc proposé : {u} <-> {v}")
                print(f"1 : {u} -> {v}")
                print(f"2 : {v} -> {u}")
                print(f"3 : Deux sens")
                choice = input("Choix (1/2/3) : ")
                if choice in ["1", "2", "3"]:
                    break
                print("Choix invalide")

            p1, p2 = nodes[u], nodes[v]

            if choice == "1":
                arcs_to_add = [(u, v, p1, p2)]
            elif choice == "2":
                arcs_to_add = [(v, u, p2, p1)]
            else:
                arcs_to_add = [(u, v, p1, p2), (v, u, p2, p1)]

            for u_final, v_final, pt1, pt2 in arcs_to_add:
                length, cx, cy = compute_arc_geometry(pt1, pt2, radius)
                writer.writerow({
                    "u": u_final,
                    "v": v_final,
                    "radius": radius if not math.isinf(radius) else "inf",
                    "length": length,
                    "cx": "" if cx is None else cx,
                    "cy": "" if cy is None else cy
                })

    print(f"✅ Orientation terminée. Fichier : {oriented_file}")


# === MAIN ===
if __name__ == "__main__":
    # Étape 1
    clean_graph(NODES_FILE, ARCS_FILE)
    # Étape 2
    orient_graph(NODES_CLEAN_FILE, ARCS_CLEAN_FILE, ARCS_ORIENTED_FILE)
