import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
import networkx as nx
import json
import csv
import math

# MODIFIÉ : J'utilise le fichier 'arcs_oriented.csv' par défaut
# pour être cohérent avec l'appel dans __main__
NODES_FILE = "nodes.csv"
ARCS_FILE = "arcs_oriented.csv" # Changé de "arcs.csv"

# --- Fonctions de chargement (inchangées) ---

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
        ax.plot([x1, x2], [y1, y2], **kwargs)
        return

    dx = x2 - x1
    dy = y2 - y1
    d = math.hypot(dx, dy)

    if d > 2 * abs(radius):
        ax.plot([x1, x2], [y1, y2], linestyle="--", color="red")
        return
    
    if d == 0:
        return

    mx, my = (x1 + x2) / 2, (y1 + y2) / 2
    
    h_sq = radius**2 - (d / 2)**2
    if h_sq < -1e-9:
        ax.plot([x1, x2], [y1, y2], linestyle="--", color="orange")
        return
    h = math.sqrt(max(0, h_sq))

    nx, ny = -dy / d, dx / d

    cx = mx + (h if radius > 0 else -h) * nx
    cy = my + (h if radius > 0 else -h) * ny

    ang1 = math.atan2(y1 - cy, x1 - cx)
    ang2 = math.atan2(y2 - cy, x2 - cx)

    if radius > 0 and ang2 < ang1:
        ang2 += 2 * math.pi
    elif radius < 0 and ang2 > ang1:
        ang2 -= 2 * math.pi

    num_points = 80
    angles = [ang1 + t * (ang2 - ang1) / (num_points - 1) for t in range(num_points)]
    xs = [cx + abs(radius) * math.cos(a) for a in angles]
    ys = [cy + abs(radius) * math.sin(a) for a in angles]

    ax.plot(xs, ys, **kwargs)

# --- Fonctions d'aide géométrique (inchangées) ---

def get_arc_center(p1, p2, radius):
    x1, y1 = p1
    x2, y2 = p2
    dx = x2 - x1
    dy = y2 - y1
    d = math.hypot(dx, dy)
    if d == 0: return None
    if d > 2 * abs(radius): return None
    mx, my = (x1 + x2) / 2, (y1 + y2) / 2
    h_sq = radius**2 - (d / 2)**2
    if h_sq < -1e-9: return None
    h = math.sqrt(max(0, h_sq))
    nx, ny = -dy / d, dx / d
    cx = mx + (h if radius > 0 else -h) * nx
    cy = my + (h if radius > 0 else -h) * ny
    return (cx, cy)

def project_point_on_segment(p, a, b):
    p = np.array(p)
    a = np.array(a)
    b = np.array(b)
    v = b - a
    w = p - a
    v_dot_v = np.dot(v, v)
    if v_dot_v == 0:
        p_proj = a
    else:
        t = np.dot(w, v) / v_dot_v
        t = np.clip(t, 0, 1)
        p_proj = a + t * v
    dist = np.linalg.norm(p - p_proj)
    return p_proj, dist

def project_point_on_arc(p, p1, p2, radius):
    p = np.array(p)
    p1 = np.array(p1)
    p2 = np.array(p2)
    center = get_arc_center(p1, p2, radius)
    if center is None:
        return project_point_on_segment(p, p1, p2)
    cx, cy = center
    abs_radius = abs(radius)
    v_p = p - center
    norm_v_p = np.linalg.norm(v_p)
    if norm_v_p == 0:
        p_proj_circle = p1
    else:
        p_proj_circle = center + abs_radius * (v_p / norm_v_p)
    ang1 = math.atan2(p1[1] - cy, p1[0] - cx)
    ang2 = math.atan2(p2[1] - cy, p2[0] - cx)
    ang_proj = math.atan2(p_proj_circle[1] - cy, p_proj_circle[0] - cx)
    ang1_norm = ang1
    ang2_norm = ang2
    ang_proj_norm = ang_proj
    is_between = False
    if radius > 0:
        if ang2_norm < ang1_norm: ang2_norm += 2 * math.pi
        if ang_proj_norm < ang1_norm: ang_proj_norm += 2 * math.pi
        is_between = (ang1_norm <= ang_proj_norm <= ang2_norm)
    else:
        if ang2_norm > ang1_norm: ang2_norm -= 2 * math.pi
        if ang_proj_norm > ang1_norm: ang_proj_norm -= 2 * math.pi
        is_between = (ang2_norm <= ang_proj_norm <= ang1_norm)
    if is_between:
        p_proj = p_proj_circle
    else:
        dist1 = np.linalg.norm(p - p1)
        dist2 = np.linalg.norm(p - p2)
        p_proj = p1 if dist1 <= dist2 else p2
    dist = np.linalg.norm(p - p_proj)
    return p_proj, dist

def get_arc_segment_length(p1, p2, radius):
    p1 = np.array(p1)
    p2 = np.array(p2)
    if math.isinf(radius) or radius == 0:
        return np.linalg.norm(p2 - p1)
    center = get_arc_center(p1, p2, radius)
    if center is None:
        return np.linalg.norm(p2 - p1)
    cx, cy = center
    ang1 = math.atan2(p1[1] - cy, p1[0] - cx)
    ang2 = math.atan2(p2[1] - cy, p2[0] - cx)
    if radius > 0 and ang2 < ang1:
        ang2 += 2 * math.pi
    elif radius < 0 and ang2 > ang1:
        ang2 -= 2 * math.pi
    d_angle = abs(ang2 - ang1)
    return abs(radius) * d_angle

# --- FIN DES FONCTIONS D'AIDE ---


# --- MODIFIÉ : Retrait de 'localisation_destination' des arguments ---
def calcul_itin(fichier_csv, fichier_arcs, localisation_voiture, output_file="itineraire.csv"):
    
    data = pd.read_csv(fichier_csv)
    arcs_data = pd.read_csv(fichier_arcs)
    points = data[['x', 'y']].to_numpy()

    # === Tracer le circuit ===
    nodes = load_nodes(fichier_csv)
    arcs = load_arcs(fichier_arcs)

    fig, ax = plt.subplots(figsize=(8, 8))
    ax.set_aspect("equal", adjustable="datalim")
    ax.invert_yaxis()

    for (u, v, r, length, cx, cy) in arcs:
        if u not in nodes or v not in nodes: continue
        p1, p2 = nodes[u], nodes[v]
        draw_arc(ax, p1, p2, r, color="green" if not math.isinf(r) else "cyan", linewidth=2)

    xs, ys = zip(*[nodes[i] for i in nodes])
    ax.scatter(xs, ys, s=30, color="blue", zorder=5)

    for i, (x, y) in nodes.items():
        ax.text(x+5, y+5, str(i), color="yellow", fontsize=8, weight="bold",
                bbox=dict(facecolor="black", alpha=0.4, pad=1))

    # Position du véhicule
    voiture_xy = np.array([localisation_voiture["x"] , localisation_voiture["y"] ])
    plt.scatter(voiture_xy[0], voiture_xy[1], color='red', s=100, label='Position voiture (Départ)', zorder=6)

    # === Logique de projection du départ (inchangée) ===
    min_dist_start = float('inf')
    best_arc_start = None
    proj_point_start = None

    for arc in arcs:
        u, v, radius, length, cx, cy = arc
        if u not in nodes or v not in nodes: continue
        p1 = nodes[u]
        p2 = nodes[v]
        if math.isinf(radius) or radius == 0:
            p_proj, dist = project_point_on_segment(voiture_xy, p1, p2)
        else:
            p_proj, dist = project_point_on_arc(voiture_xy, p1, p2, radius)
        if dist < min_dist_start:
            min_dist_start = dist
            best_arc_start = arc
            proj_point_start = p_proj

    if proj_point_start is not None:
        ax.scatter(proj_point_start[0], proj_point_start[1], color='orange', s=50, zorder=6, label='Point de départ (projeté)')
        ax.plot([voiture_xy[0], proj_point_start[0]], [voiture_xy[1], proj_point_start[1]], 'r--', zorder=6)
    
    
    # --- AJOUTÉ : Sélection interactive de la destination ---
    print("Veuillez cliquer sur le graphique pour choisir la destination...")
    ax.set_title("Visualisation - VEUILLEZ CLIQUER POUR CHOISIR LA DESTINATION")
    ax.legend() # Afficher la légende avant le ginput
    
    # Afficher la fenêtre (nécessaire pour ginput)
    plt.draw() 
    
    # Capture du clic
    # plt.ginput(1) retourne une liste de tuples, ex: [(123.4, 567.8)]
    selected_point = plt.ginput(1, timeout=-1) # Attente indéfinie
    
    if not selected_point:
        print("Aucun point cliqué ou fenêtre fermée. Annulation.")
        plt.close(fig)
        return []
        
    # On prend le premier élément [0]
    dest_xy = np.array(selected_point[0])
    
    print(f"Destination choisie (x, y) : {dest_xy}")
    
    # Afficher le point de destination choisi
    plt.scatter(dest_xy[0], dest_xy[1], color='purple', s=100, marker='X', label='Destination (Choisie)', zorder=6)
    ax.set_title("Visualisation du graphe exporté") # Remettre le titre normal
    ax.legend() # Mettre à jour la légende
    # --- Fin de la partie ajoutée ---

    # Afficher le graphique final (non bloquant, ou bloquant si désiré)
    print("Affichage du graphique final... Fermez la fenêtre pour continuer le calcul.")
    plt.show(block=True) # Mettre 'block=True' pour forcer la fermeture

    # === Créer un graphe avec les points du circuit ===
    G = nx.DiGraph()
    
    for _, row in data.iterrows():
        node_id = int(row['id'])
        pos_tuple = (float(row['x']), float(row['y']))
        G.add_node(node_id, pos=pos_tuple)

    arcs_info = {}
    for _, row in arcs_data.iterrows():
        u = int(row['u'])
        v = int(row['v'])
        length = float(row['length'])
        radius = row['radius']
        G.add_edge(u, v, weight=length, radius=radius)
        arcs_info[(u, v)] = {
            'length': length,
            'radius': radius if radius != 'inf' else float('inf'),
            'cx': row['cx'] if pd.notna(row['cx']) else None,
            'cy': row['cy'] if pd.notna(row['cy']) else None
        }

    def two_closest_indices(p, pts):
        dists = np.linalg.norm(pts - p, axis=1)
        return np.argsort(dists)[:2]

    node_id_list = data['id'].to_numpy(dtype=int)

    indices_selected = []
    
    max_id_from_nodes = node_id_list.max()
    all_node_ids_in_graph = list(G.nodes())
    max_id_from_graph = max(all_node_ids_in_graph) if all_node_ids_in_graph else max_id_from_nodes
    max_id = max(max_id_from_nodes, max_id_from_graph)
    
    # === Logique du point de départ (inchangée) ===
    if best_arc_start is None:
        print("Erreur : Aucun arc trouvé pour le départ. Vérifiez les fichiers.")
        return []

    u, v, radius, length, _, _ = best_arc_start
    p1 = nodes[u]
    p2 = nodes[v]
    len_u_proj = get_arc_segment_length(p1, proj_point_start, radius)
    len_proj_v = get_arc_segment_length(proj_point_start, p2, radius)
    
    idx_proj_start = max_id + 1
    idx_start = max_id + 2
    
    G.add_node(idx_proj_start, pos=tuple(proj_point_start))
    G.add_node(idx_start, pos=tuple(voiture_xy))
    
    if G.has_edge(u, v):
        G.remove_edge(u, v)
        
    G.add_edge(u, idx_proj_start, weight=len_u_proj, radius=radius)
    G.add_edge(idx_proj_start, v, weight=len_proj_v, radius=radius)
    G.add_edge(idx_start, idx_proj_start, weight=min_dist_start, radius=float('inf'))

    indices_selected.append(idx_start)

    # === Logique du point d'arrivée (inchangée) ===
    # MAIS utilise maintenant 'dest_xy' défini interactivement
    idx_end = max_id + 3
    G.add_node(idx_end, pos=dest_xy)
    
    for ci in two_closest_indices(dest_xy, points):
        closest_node_id = node_id_list[ci]
        dist = np.linalg.norm(dest_xy - points[ci])
        G.add_edge(idx_end, closest_node_id, weight=dist, radius=float('inf'))
        G.add_edge(closest_node_id, idx_end, weight=dist, radius=float('inf'))
        
    indices_selected.append(idx_end)
    # --- Fin de la modification ---


    # === Calcul du plus court chemin ===
    start, end = indices_selected
    try:
        shortest_path = nx.shortest_path(G, source=start, target=end, weight='weight')
    except nx.NetworkXNoPath:
        print(f"Erreur : Aucun chemin trouvé entre le départ {start} et la destination {end}.")
        return []

    # === Visualisation (inchangée) ===
    pos = nx.get_node_attributes(G, 'pos')
    path_coords = np.array([pos[i] for i in shortest_path])

    # === Conversion CSV (inchangée) ===
    def calculer_theta(p1, p2):
        dx = p2[0] - p1[0]
        dy = p2[1] - p1[1]
        return np.degrees(np.arctan2(dy, dx))

    itineraire_points = []
    for i in range(len(path_coords)):
        node_index = shortest_path[i]
        x_mm = int(path_coords[i][0])
        y_mm = int(path_coords[i][1])
        z_mm = 0
        if i < len(path_coords) - 1:
            theta = float(calculer_theta(path_coords[i], path_coords[i + 1]))
        else:
            theta = 0.0
        itineraire_points.append({
            "id": node_index,
            "x": x_mm,
            "y": y_mm,
            "z": z_mm,
            "theta": theta
        })

    # === Sauvegarde CSV (inchangée) ===
    with open(output_file, "w", newline="", encoding="utf-8") as csvfile:
        fieldnames = ["id", "x", "y", "z", "theta"]
        writer = csv.DictWriter(csvfile, fieldnames=fieldnames)
        writer.writeheader()
        writer.writerows(itineraire_points)

    print(f"✅ Fichier CSV généré et enregistré dans {output_file}")

    print("🔹 Chemin (indices des noeuds) :", shortest_path)
    print("🔹 Longueur totale du chemin :", nx.path_weight(G, shortest_path, weight='weight'))

    return itineraire_points


if __name__ == "__main__":
    # La destination (position_voiture_init) n'est plus nécessaire ici
    # position_voiture_init = {"theta":0, "x":1485.0,"y" : 603.0, "z":0} 
    position_voiture = {"theta":0, "x":387.0, "y":731.0
, "z":0}      # Départ
    
    # --- MODIFIÉ : Retrait de l'argument 'localisation_destination' ---
    calcul_itin("nodes.csv", 
                "arcs_oriented.csv", 
                position_voiture,       # localisation_voiture
                output_file="itineraire.csv")