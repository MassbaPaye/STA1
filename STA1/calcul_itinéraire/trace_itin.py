import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
import networkx as nx
import json
import csv
import math

# --- Configuration Globale ---
NODES_FILE = "nodes.csv"
ARCS_FILE = "arcs_oriented.csv"
STEP_SIZE_MM = 50
SPARSE_OUTPUT_FILE = "itineraire.csv"
DENSE_OUTPUT_FILE = "itineraire_dense.csv"

def load_nodes(path):
    """Charge les nœuds (nodes.csv) pour le tracé."""
    nodes = {}
    with open(path, newline='', encoding="utf-8") as f:
        reader = csv.DictReader(f)
        for row in reader:
            nodes[int(row["id"])] = (float(row["x"]), float(row["y"]))
    return nodes

def load_arcs(path):
    """Charge la version complète des arcs (avec length, cx, cy)."""
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
    """Dessine un segment ou un arc circulaire (recalcule le centre)."""
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

def get_arc_center(p1, p2, radius):
    """Calcule le centre d'un arc (utilisé par projection ET interpolation)."""
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
    """Projte le point p sur le segment [a, b]."""
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
    """Projete le point p sur l'arc de cercle (p1, p2, radius)."""
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
    """Calcule la longueur d'un segment ou d'un arc."""
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

def interpolate_path(sparse_path, arc_lookup, step_size):
    """Interpole le chemin clairsemé en utilisant la géométrie p1, p2, radius."""
    dense_points_xy = []
    if not sparse_path:
        return []
    p1 = sparse_path[0]
    # Stocke (x, y, pont_flag, depass_flag)
    dense_points_xy.append((p1['x'], p1['y'], 0, 0))

    for i in range(len(sparse_path) - 1):
        p1 = sparse_path[i]
        p2 = sparse_path[i+1]
        u, v = p1['id'], p2['id']
        
        arc_info = arc_lookup.get((u, v)) 
        
        pont_flag = 0
        depass_flag = 0 
        if arc_info:
            pont_flag = arc_info.get('pont', 0)
            depass_flag = arc_info.get('depass', 0) 

        p1_xy = np.array([p1['x'], p1['y']])
        p2_xy = np.array([p2['x'], p2['y']])
        p1_tuple = (p1['x'], p1['y'])
        p2_tuple = (p2['x'], p2['y'])

        is_straight_line = True
        center_tuple = None
        radius = float('inf')

        if arc_info and not math.isinf(arc_info['radius']):
            radius = arc_info['radius']
            center_tuple = get_arc_center(p1_tuple, p2_tuple, radius)
            if center_tuple:
                is_straight_line = False

        if is_straight_line:
            dist = np.linalg.norm(p2_xy - p1_xy)
            if dist == 0: continue
            num_steps = max(2, int(dist / step_size) + 1)
            xs = np.linspace(p1['x'], p2['x'], num_steps)
            ys = np.linspace(p1['y'], p2['y'], num_steps)
            for j in range(1, num_steps):
                dense_points_xy.append((xs[j], ys[j], pont_flag, depass_flag)) 
        else:
            cx, cy = center_tuple
            abs_radius = abs(radius)
            ang1 = math.atan2(p1['y'] - cy, p1['x'] - cx)
            ang2 = math.atan2(p2['y'] - cy, p2['x'] - cx)
            if radius > 0 and ang2 < ang1: ang2 += 2 * math.pi
            elif radius < 0 and ang2 > ang1: ang2 -= 2 * math.pi
            d_angle = ang2 - ang1
            arc_length = abs_radius * abs(d_angle)
            if arc_length == 0: continue
            num_steps = max(2, int(arc_length / step_size) + 1)
            angles = np.linspace(ang1, ang2, num_steps)
            for j in range(1, num_steps):
                a = angles[j]
                x = cx + abs_radius * math.cos(a)
                y = cy + abs_radius * math.sin(a)
                dense_points_xy.append((x, y, pont_flag, depass_flag)) 

    final_trajectory = []
    for k in range(len(dense_points_xy)):
        current_p = dense_points_xy[k]
        x, y, pont_flag, depass_flag = current_p[0], current_p[1], current_p[2], current_p[3] 
        
        if k < len(dense_points_xy) - 1:
            next_p = dense_points_xy[k+1]
            dx = next_p[0] - x
            dy = next_p[1] - y
            theta = math.degrees(math.atan2(dy, dx))
        else:
            theta = final_trajectory[-1]['theta'] if final_trajectory else 0.0
            
        final_trajectory.append({
            'id': k,
            'x': round(x, 4),
            'y': round(y, 4),
            'z': 0,
            'theta': round(theta, 4),
            'pont': pont_flag,
            'depass': depass_flag 
        })
    return final_trajectory

def save_dense_path(file_path, trajectory):
    """Sauvegarde la trajectoire dense finale dans un fichier CSV."""
    if not trajectory:
        print("Avertissement : Trajectoire vide, aucun fichier de sortie généré.")
        return
    with open(file_path, "w", newline="", encoding="utf-8") as csvfile:
        fieldnames = ["id", "x", "y", "z", "theta", "pont", "depass"] 
        writer = csv.DictWriter(csvfile, fieldnames=fieldnames)
        writer.writeheader()
        writer.writerows(trajectory)
    print(f"✅ Trajectoire dense générée et enregistrée dans {file_path}")
    print(f"   (Contient {len(trajectory)} points)")

def visualize_final_path(dense_trajectory, nodes_file, arcs_file):
    """Affiche la carte de base et la trajectoire dense finale."""
    if not dense_trajectory:
        print("Trajectoire vide, affichage annulé.")
        return
        
    print("Affichage du graphique de la trajectoire dense...")
    
    nodes = load_nodes(nodes_file)
    arcs_full = load_arcs(arcs_file) 

    fig, ax = plt.subplots(figsize=(10, 10))
    ax.set_aspect("equal", adjustable="datalim")
    ax.invert_yaxis()

    for (u, v, r, _, _, _) in arcs_full:
        if u not in nodes or v not in nodes: continue
        p1, p2 = nodes[u], nodes[v]
        # Note : On ne peut pas récupérer BRIDGE_ARCS et DEPASS ici facilement
        # sans les passer en paramètre, donc on garde l'affichage de base.
        draw_arc(ax, p1, p2, r, color="green" if not math.isinf(r) else "cyan", linewidth=1.5, alpha=0.5)

    if nodes:
        xs, ys = zip(*nodes.values())
        ax.scatter(xs, ys, s=20, color="blue", zorder=5, alpha=0.6)
    
    dense_xs = [p['x'] for p in dense_trajectory]
    dense_ys = [p['y'] for p in dense_trajectory]
    
    pont_xs = [p['x'] for p in dense_trajectory if p['pont'] == 1]
    pont_ys = [p['y'] for p in dense_trajectory if p['pont'] == 1]
    
    depass_xs = [p['x'] for p in dense_trajectory if p['depass'] == 1] 
    depass_ys = [p['y'] for p in dense_trajectory if p['depass'] == 1] 

    if pont_xs:
        ax.scatter(pont_xs, pont_ys, color='gray', s=15, zorder=9, label='Pont')
        
    if depass_xs: 
        ax.scatter(depass_xs, depass_ys, color='orange', s=15, zorder=9, label='Dépassement') 

    ax.plot(dense_xs, dense_ys, color='magenta', linewidth=2.5, zorder=10, label='Itinéraire Dense')
    
    ax.scatter([dense_xs[0]], [dense_ys[0]], color='red', s=100, zorder=11, label='Départ')
    ax.scatter([dense_xs[-1]], [dense_ys[-1]], color='cyan', s=100, marker='X', zorder=11, label='Arrivée (Projetée)')

    ax.set_title("Visualisation de l'Itinéraire Dense")
    ax.legend()
    plt.grid(True, linestyle='--', alpha=0.3)
    
    print("Affichage du graphique final... Fermez la fenêtre pour terminer.")
    plt.show()

def calcul_et_interpolation_itin(fichier_csv, fichier_arcs, localisation_voiture, sparse_output_file, dense_output_file):
    """
    Fonction principale qui orchestre le calcul de chemin,
    la sélection interactive, et l'interpolation.
    """
    
    # DÉFINITION DES ARCS DU PONT ET DES ZONES DE DEPASSEMENT
    BRIDGE_ARCS = {
        (23, 24), (24, 23), (23,21), (21,23), (21,22), (37,24), (24,25), (15, 16), (17,14)
    }

    DEPASS = {
        (20,19), (18,21), (29,30), (30,31), (31,32), (32,10), (11,33), (33,34), (34,35), (35,55), (55,36)
    }
    
    data = pd.read_csv(fichier_csv)
    arcs_data = pd.read_csv(fichier_arcs)
    nodes = load_nodes(fichier_csv)
    arcs = load_arcs(fichier_arcs)
        
    points = data[['x', 'y']].to_numpy()

    # 1 : Tracé interactif et sélection 
    fig, ax = plt.subplots(figsize=(8, 8))
    ax.set_aspect("equal", adjustable="datalim")
    ax.invert_yaxis()

    for (u, v, r, length, cx, cy) in arcs:
        if u not in nodes or v not in nodes: continue
        p1, p2 = nodes[u], nodes[v]
        is_bridge = (u, v) in BRIDGE_ARCS
        is_depass = (u, v) in DEPASS
        color = "cyan"
        if is_bridge:
            color = "gray"
        elif is_depass:
            color = "orange" 
        elif not math.isinf(r):
            color = "green"  
        alpha = 0.8 if is_bridge or is_depass else 1.0 
        draw_arc(ax, p1, p2, r, color=color, linewidth=2, alpha=alpha)

    xs, ys = zip(*[nodes[i] for i in nodes])
    ax.scatter(xs, ys, s=30, color="blue", zorder=5)

    voiture_xy = np.array([localisation_voiture["x"] , localisation_voiture["y"] ])
    plt.scatter(voiture_xy[0], voiture_xy[1], color='red', s=100, label='Position voiture (Départ)', zorder=6)

    # Projection du point de Départ sur l'arc le plus proche
    min_dist_start = float('inf')
    best_arc_start = None
    proj_point_start = None
    for arc in arcs:
        u, v, radius, length, cx, cy = arc
        if u not in nodes or v not in nodes: continue
        p1, p2 = nodes[u], nodes[v]
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
    else:
        print("Erreur : Impossible de projeter le point de départ.")
        plt.close(fig)
        return None
    
    # Sélection interactive de la destination
    print("Veuillez cliquer sur le graphique pour choisir la destination...")
    ax.set_title("VEUILLEZ CLIQUER POUR CHOISIR LA DESTINATION")
    ax.legend()
    plt.draw() 
    
    selected_point = plt.ginput(1, timeout=-1)
    
    if not selected_point:
        print("Aucun point cliqué. Annulation.")
        plt.close(fig)
        return None
        
    dest_xy = np.array(selected_point[0])
    print(f"Destination choisie (x, y) : {dest_xy}")
    plt.scatter(dest_xy[0], dest_xy[1], color='purple', s=100, marker='X', label='Destination (Choisie)', zorder=6)

    # --- Projection de la destination ---
    min_dist_end = float('inf')
    best_arc_end = None
    proj_point_end = None
    for arc in arcs:
        u, v, radius, length, cx, cy = arc
        if u not in nodes or v not in nodes: continue
        p1, p2 = nodes[u], nodes[v]
        if math.isinf(radius) or radius == 0:
            p_proj, dist = project_point_on_segment(dest_xy, p1, p2)
        else:
            p_proj, dist = project_point_on_arc(dest_xy, p1, p2, radius)
        if dist < min_dist_end:
            min_dist_end = dist
            best_arc_end = arc
            proj_point_end = p_proj

    if proj_point_end is not None:
        ax.scatter(proj_point_end[0], proj_point_end[1], color='cyan', s=50, zorder=6, label="Point d'arrivée (projeté)")
        ax.plot([dest_xy[0], proj_point_end[0]], [dest_xy[1], proj_point_end[1]], 'm--', zorder=6)
    else:
        print("Erreur : Impossible de projeter le point de destination.")
        plt.close(fig)
        return None
    
    ax.set_title("Calcul en cours...")
    ax.legend()
    plt.draw()
    print("Projections calculées. Fermeture de la fenêtre interactive...")
    plt.pause(1.0)
    plt.close(fig)

    # 2 : Création et modification du graphe 
    G = nx.DiGraph()
    for _, row in data.iterrows():
        node_id = int(row['id'])
        pos_tuple = (float(row['x']), float(row['y']))
        G.add_node(node_id, pos=pos_tuple)

    for _, row in arcs_data.iterrows():
        u = int(row['u'])
        v = int(row['v'])
        length = float(row['length'])
        radius = float("inf") if row["radius"] == "inf" else float(row["radius"])
        
        pont = 1 if (u, v) in BRIDGE_ARCS else 0
        deppass = 1 if (u, v) in DEPASS else 0
            
        G.add_edge(u, v, weight=length, radius=radius, pont=pont, depass=deppass) 

    node_id_list = data['id'].to_numpy(dtype=int)
    
    max_id = max(list(G.nodes()))
    
    idx_proj_start = max_id + 1
    idx_start = max_id + 2
    idx_proj_end = max_id + 3 
    
    G.add_node(idx_start, pos=tuple(voiture_xy))
    G.add_node(idx_proj_start, pos=tuple(proj_point_start))
    G.add_node(idx_proj_end, pos=tuple(proj_point_end))
    
    G.add_edge(idx_start, idx_proj_start, weight=min_dist_start, radius=float('inf'), pont=0, depass=0) 
    
    u_s, v_s, r_s, _, _, _ = best_arc_start
    u_e, v_e, r_e, _, _, _ = best_arc_end
    
    # Récupération des attributs
    attrs_s = G.edges[u_s, v_s] if G.has_edge(u_s, v_s) else {
        'radius': r_s, 
        'pont': 1 if (u_s, v_s) in BRIDGE_ARCS else 0,
        'depass': 1 if (u_s, v_s) in DEPASS else 0 
    }
    attrs_e = G.edges[u_e, v_e] if G.has_edge(u_e, v_e) else {
        'radius': r_e, 
        'pont': 1 if (u_e, v_e) in BRIDGE_ARCS else 0,
        'depass': 1 if (u_e, v_e) in DEPASS else 0 
    }
    
    pont_s = attrs_s.get('pont', 0)
    depass_s = attrs_s.get('depass', 0) 
    pont_e = attrs_e.get('pont', 0)
    depass_e = attrs_e.get('depass', 0)

    if (u_s, v_s) == (u_e, v_e):
        print("Cas: Départ et arrivée sur le même arc.")
        u, v, radius = u_s, v_s, r_s
        p1, p2 = nodes[u], nodes[v]
        
        len_u_proj_s = get_arc_segment_length(p1, proj_point_start, radius)
        len_u_proj_e = get_arc_segment_length(p1, proj_point_end, radius)
        
        if G.has_edge(u, v):
            G.remove_edge(u, v)

        if len_u_proj_s < len_u_proj_e:
            len_s_e = get_arc_segment_length(proj_point_start, proj_point_end, radius)
            len_e_v = get_arc_segment_length(proj_point_end, p2, radius)
            
            G.add_edge(u, idx_proj_start, weight=len_u_proj_s, radius=attrs_s['radius'], pont=pont_s, depass=depass_s) 
            G.add_edge(idx_proj_start, idx_proj_end, weight=len_s_e, radius=attrs_s['radius'], pont=pont_s, depass=depass_s) 
            G.add_edge(idx_proj_end, v, weight=len_e_v, radius=attrs_s['radius'], pont=pont_s, depass=depass_s) 
        else:
            len_e_s = get_arc_segment_length(proj_point_end, proj_point_start, radius)
            len_s_v = get_arc_segment_length(proj_point_start, p2, radius)

            G.add_edge(u, idx_proj_end, weight=len_u_proj_e, radius=attrs_s['radius'], pont=pont_s, depass=depass_s) 
            G.add_edge(idx_proj_end, idx_proj_start, weight=len_e_s, radius=attrs_s['radius'], pont=pont_s, depass=depass_s) 
            G.add_edge(idx_proj_start, v, weight=len_s_v, radius=attrs_s['radius'], pont=pont_s, depass=depass_s) 
    else:
        print("Cas: Départ et arrivée sur des arcs différents.")
        
        p1_s, p2_s = nodes[u_s], nodes[v_s]
        len_u_proj_s = get_arc_segment_length(p1_s, proj_point_start, r_s)
        len_proj_v_s = get_arc_segment_length(proj_point_start, p2_s, r_s)
        
        if G.has_edge(u_s, v_s):
            G.remove_edge(u_s, v_s)
            G.add_edge(u_s, idx_proj_start, weight=len_u_proj_s, radius=attrs_s['radius'], pont=pont_s, depass=depass_s) 
            G.add_edge(idx_proj_start, v_s, weight=len_proj_v_s, radius=attrs_s['radius'], pont=pont_s, depass=depass_s) 
        
        p1_e, p2_e = nodes[u_e], nodes[v_e]
        len_u_proj_e = get_arc_segment_length(p1_e, proj_point_end, r_e)
        len_proj_v_e = get_arc_segment_length(proj_point_end, p2_e, r_e)

        if G.has_edge(u_e, v_e):
            G.remove_edge(u_e, v_e)
            G.add_edge(u_e, idx_proj_end, weight=len_u_proj_e, radius=attrs_e['radius'], pont=pont_e, depass=depass_e) 
            G.add_edge(idx_proj_end, v_e, weight=len_proj_v_e, radius=attrs_e['radius'], pont=pont_e, depass=depass_e) 
    
    indices_selected = [idx_start, idx_proj_end]
    
    # 3 : Calcul du chemin
    
    start, end = indices_selected
    try:
        shortest_path_ids = nx.shortest_path(G, source=start, target=end, weight='weight')
    except nx.NetworkXNoPath:
        print(f"Erreur : Aucun chemin trouvé entre le départ {start} et la destination {end}.")
        return None
    
    print("🔹 Chemin (indices des noeuds) :", shortest_path_ids)
    print("🔹 Longueur totale du chemin :", nx.path_weight(G, shortest_path_ids, weight='weight'))

    pos = nx.get_node_attributes(G, 'pos')
    path_coords = np.array([pos[i] for i in shortest_path_ids])
    
    itineraire_points_sparse = []
    
    def calculer_theta(p1, p2):
        dx = p2[0] - p1[0]
        dy = p2[1] - p1[1]
        return np.degrees(np.arctan2(dy, dx))

    for i in range(len(path_coords)):
        node_index = shortest_path_ids[i]
        x_mm = path_coords[i][0]
        y_mm = path_coords[i][1]
        if i < len(path_coords) - 1:
            theta = float(calculer_theta(path_coords[i], path_coords[i + 1]))
        else:
            theta = itineraire_points_sparse[-1]['theta'] if itineraire_points_sparse else 0.0
        
        itineraire_points_sparse.append({
            "id": node_index,
            "x": x_mm,
            "y": y_mm,
            "z": 0,
            "theta": theta
        })

    with open(sparse_output_file, "w", newline="", encoding="utf-8") as csvfile:
        fieldnames = ["id", "x", "y", "z", "theta"]
        writer = csv.DictWriter(csvfile, fieldnames=fieldnames)
        writer.writeheader()
        writer.writerows(itineraire_points_sparse)
    print(f"✅ Fichier CSV (sparse) généré et enregistré dans {sparse_output_file}")

    # === 4. Interpolation "Dense" ===

    print("Construction du lookup d'arcs depuis le graphe final pour l'interpolation...")
    arc_lookup_from_graph = {}
    for u, v, data in G.edges(data=True):
        arc_lookup_from_graph[(u, v)] = {
            'radius': data.get('radius', float('inf')),
            'pont': data.get('pont', 0),
            'depass': data.get('depass', 0) 
        }
    
    print(f"Interpolation de la trajectoire (pas de {STEP_SIZE_MM} mm)...")
    dense_trajectory = interpolate_path(itineraire_points_sparse, arc_lookup_from_graph, STEP_SIZE_MM) 
    
    save_dense_path(dense_output_file, dense_trajectory)

    # === 5. Visualisation Finale ===
    
    visualize_final_path(dense_trajectory, fichier_csv, fichier_arcs)
    
    # === 6. Conversion en liste de listes 
    
    if not dense_trajectory:
        print("Avertissement : Trajectoire dense vide, retour de None.")
        return None
        
    # Définir l'ordre des colonnes, identique au CSV
    fieldnames = ["id", "x", "y", "z", "theta", "pont", "depass"]
    
    # Créer la liste de listes
    list_of_lists_output = []
    
    # Ajouter l'en-tête
    list_of_lists_output.append(fieldnames)
    
    # Ajouter les données
    for point in dense_trajectory:
        row = [
            point.get('id', 0), 
            point.get('x', 0.0), 
            point.get('y', 0.0), 
            point.get('z', 0), 
            point.get('theta', 0.0), 
            point.get('pont', 0), 
            point.get('depass', 0)
        ]
        list_of_lists_output.append(row)
        
    print(f"✅ Trajectoire convertie en liste de listes (contient {len(list_of_lists_output)} lignes, en-tête inclus).")
    
    # Retourner la  structure
    return list_of_lists_output
