import csv
import math
import numpy as np
import matplotlib.pyplot as plt

# --- Configuration ---
STEP_SIZE_MM = 10.0
SPARSE_FILE = "itineraire.csv"
ARCS_FILE = "arcs_oriented.csv"
NODES_FILE = "nodes.csv"
DENSE_FILE = "itineraire_dense.csv"
# ---------------------

# --- Fonctions de chargement et de tracé de la carte ---
# (Ces fonctions sont pour la visualisation de fond)

def load_nodes(path):
    """Charge les nœuds (nodes.csv) pour le tracé."""
    nodes = {}
    with open(path, newline='', encoding="utf-8") as f:
        reader = csv.DictReader(f)
        for row in reader:
            try:
                nodes[int(row["id"])] = (float(row["x"]), float(row["y"]))
            except (ValueError, TypeError):
                print(f"Avertissement : Ligne de nœud ignorée : {row}")
    return nodes

def load_arcs(path):
    """Charge les arcs (arcs_oriented.csv) pour le tracé."""
    arcs = []
    with open(path, newline='', encoding="utf-8") as f:
        reader = csv.DictReader(f)
        for row in reader:
            try:
                u = int(row["u"])
                v = int(row["v"])
                radius = float("inf") if row["radius"] == "inf" else float(row["radius"])
                # Note : on ignore cx, cy pour le tracé, car draw_arc les recalcule
                arcs.append((u, v, radius))
            except (ValueError, TypeError):
                print(f"Avertissement : Ligne d'arc ignorée : {row}")
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

    # Gérer les cas impossibles
    if d == 0:
        return
    if d > 2 * abs(radius):
        ax.plot([x1, x2], [y1, y2], linestyle="--", color="red") # Arc impossible
        return

    mx, my = (x1 + x2) / 2, (y1 + y2) / 2
    
    h_sq = radius**2 - (d / 2)**2
    if h_sq < -1e-9: # Tolérance
        ax.plot([x1, x2], [y1, y2], linestyle="--", color="orange") # Arc presque impossible
        return
    h = math.sqrt(max(0, h_sq))

    nx, ny = -dy / d, dx / d

    # Calcul du centre
    cx = mx + (h if radius > 0 else -h) * nx
    cy = my + (h if radius > 0 else -h) * ny

    ang1 = math.atan2(y1 - cy, x1 - cx)
    ang2 = math.atan2(y2 - cy, x2 - cx)

    # Gestion de la direction
    if radius > 0 and ang2 < ang1:
        ang2 += 2 * math.pi
    elif radius < 0 and ang2 > ang1:
        ang2 -= 2 * math.pi

    num_points = 80
    angles = [ang1 + t * (ang2 - ang1) / (num_points - 1) for t in range(num_points)]
    xs = [cx + abs(radius) * math.cos(a) for a in angles]
    ys = [cy + abs(radius) * math.sin(a) for a in angles]

    ax.plot(xs, ys, **kwargs)

# --- NOUVELLE FONCTION (copiée du script précédent) ---
def get_arc_center(p1, p2, radius):
    """
    Calcule le centre d'un arc à partir de p1, p2 et radius.
    C'est la même logique que draw_arc.
    Retourne (cx, cy) ou None si l'arc est impossible.
    """
    x1, y1 = p1
    x2, y2 = p2
    dx = x2 - x1
    dy = y2 - y1
    d = math.hypot(dx, dy)

    if d == 0: return None
    # Gérer les cas impossibles
    if d > 2 * abs(radius): return None
    
    mx, my = (x1 + x2) / 2, (y1 + y2) / 2
    
    h_sq = radius**2 - (d / 2)**2
    if h_sq < -1e-9: return None # Tolérance
    h = math.sqrt(max(0, h_sq))

    nx, ny = -dy / d, dx / d
    cx = mx + (h if radius > 0 else -h) * nx
    cy = my + (h if radius > 0 else -h) * ny
    return (cx, cy)

# --- Fonctions d'interpolation (MISES À JOUR) ---

def load_sparse_path(file_path):
    """Charge le fichier d'itinéraire clairsemé (ex: itineraire.csv)."""
    path = []
    with open(file_path, 'r', encoding='utf-8') as f:
        reader = csv.DictReader(f)
        for row in reader:
            path.append({
                'id': int(row['id']),
                'x': float(row['x']),
                'y': float(row['y']),
                'z': float(row['z']),
                'theta': float(row['theta'])
            })
    return path

def load_arc_data(file_path):
    """
    Charge les données des arcs (arcs_oriented.csv).
    Note : On n'a plus besoin de cx, cy.
    """
    arc_lookup = {}
    with open(file_path, 'r', encoding='utf-8') as f:
        reader = csv.DictReader(f)
        for row in reader:
            try:
                u = int(row['u'])
                v = int(row['v'])
                arc_lookup[(u, v)] = {
                    'radius': float('inf') if row['radius'] == 'inf' else float(row['radius'])
                }
            except (ValueError, TypeError):
                print(f"Avertissement : Ligne d'arc ignorée (données invalides) : {row}")
    return arc_lookup

def interpolate_path(sparse_path, arc_lookup, step_size):
    """
    Interpole le chemin clairsemé en utilisant la géométrie p1, p2, radius.
    """
    
    dense_points_xy = []
    if not sparse_path:
        return []
    p1 = sparse_path[0]
    dense_points_xy.append((p1['x'], p1['y']))

    for i in range(len(sparse_path) - 1):
        p1 = sparse_path[i]
        p2 = sparse_path[i+1]
        
        u, v = p1['id'], p2['id']
        arc_info = arc_lookup.get((u, v))

        p1_xy = np.array([p1['x'], p1['y']])
        p2_xy = np.array([p2['x'], p2['y']])
        p1_tuple = (p1['x'], p1['y'])
        p2_tuple = (p2['x'], p2['y'])

        is_straight_line = True
        center_tuple = None
        radius = float('inf')

        if arc_info and not math.isinf(arc_info['radius']):
            radius = arc_info['radius']
            # --- MODIFICATION CLÉ : Recalculer le centre ---
            center_tuple = get_arc_center(p1_tuple, p2_tuple, radius)
            if center_tuple:
                is_straight_line = False

        # --- Cas 1 : Ligne Droite ---
        # (Soit 'inf', soit arc impossible (rayon trop petit), etc.)
        if is_straight_line:
            dist = np.linalg.norm(p2_xy - p1_xy)
            if dist == 0:
                continue
            
            num_steps = max(2, int(dist / step_size) + 1)
            xs = np.linspace(p1['x'], p2['x'], num_steps)
            ys = np.linspace(p1['y'], p2['y'], num_steps)
            
            for j in range(1, num_steps):
                dense_points_xy.append((xs[j], ys[j]))

        # --- Cas 2 : Arc de Cercle ---
        else:
            cx, cy = center_tuple
            abs_radius = abs(radius)

            ang1 = math.atan2(p1['y'] - cy, p1['x'] - cx)
            ang2 = math.atan2(p2['y'] - cy, p2['x'] - cx)

            if radius > 0 and ang2 < ang1:
                ang2 += 2 * math.pi
            elif radius < 0 and ang2 > ang1:
                ang2 -= 2 * math.pi

            d_angle = ang2 - ang1
            arc_length = abs_radius * abs(d_angle)
            
            if arc_length == 0:
                continue

            num_steps = max(2, int(arc_length / step_size) + 1)
            angles = np.linspace(ang1, ang2, num_steps)
            
            for j in range(1, num_steps):
                a = angles[j]
                x = cx + abs_radius * math.cos(a)
                y = cy + abs_radius * math.sin(a)
                dense_points_xy.append((x, y))

    # --- Étape finale : Calculer Theta et formater la sortie ---
    final_trajectory = []
    for k in range(len(dense_points_xy)):
        current_p = dense_points_xy[k]
        x, y = current_p[0], current_p[1]

        if k < len(dense_points_xy) - 1:
            next_p = dense_points_xy[k+1]
            dx = next_p[0] - x
            dy = next_p[1] - y
            theta = math.degrees(math.atan2(dy, dx))
        else:
            theta = final_trajectory[-1]['theta'] if final_trajectory else 0.0

        final_trajectory.append({
            'id': k,
            'x': round(x, 4), # Arrondir pour la propreté
            'y': round(y, 4),
            'z': 0,
            'theta': round(theta, 4)
        })
        
    return final_trajectory

def save_dense_path(file_path, trajectory):
    """Sauvegarde la trajectoire dense finale dans un fichier CSV."""
    if not trajectory:
        print("Avertissement : Trajectoire vide, aucun fichier de sortie généré.")
        return

    with open(file_path, "w", newline="", encoding="utf-8") as csvfile:
        fieldnames = ["id", "x", "y", "z", "theta"]
        writer = csv.DictWriter(csvfile, fieldnames=fieldnames)
        writer.writeheader()
        writer.writerows(trajectory)
    print(f"✅ Trajectoire dense générée et enregistrée dans {file_path}")
    print(f"   (Contient {len(trajectory)} points)")


if __name__ == "__main__":
    print(f"Chargement du chemin clairsemé depuis '{SPARSE_FILE}'...")
    sparse_path = load_sparse_path(SPARSE_FILE)
    
    print(f"Chargement des données d'arcs depuis '{ARCS_FILE}'...")
    arc_lookup = load_arc_data(ARCS_FILE)
    
    print(f"Interpolation de la trajectoire (pas de {STEP_SIZE_MM} mm)...")
    dense_trajectory = interpolate_path(sparse_path, arc_lookup, STEP_SIZE_MM)
    
    save_dense_path(DENSE_FILE, dense_trajectory)

    # --- Section de visualisation (inchangée, mais utilise le nouveau draw_arc) ---
    if not dense_trajectory:
        print("Trajectoire vide, affichage annulé.")
    else:
        print("Affichage du graphique de la trajectoire dense...")
        
        nodes = load_nodes(NODES_FILE)
        arcs = load_arcs(ARCS_FILE) 

        fig, ax = plt.subplots(figsize=(10, 10))
        ax.set_aspect("equal", adjustable="datalim")
        ax.invert_yaxis()

        # 1. Tracer la carte de base (arcs)
        for (u, v, r) in arcs:
            if u not in nodes or v not in nodes: continue
            p1, p2 = nodes[u], nodes[v]
            draw_arc(ax, p1, p2, r, color="green" if not math.isinf(r) else "cyan", linewidth=1.5, alpha=0.5)

        # 2. Tracer la carte de base (nœuds)
        if nodes:
            xs, ys = zip(*nodes.values())
            ax.scatter(xs, ys, s=20, color="blue", zorder=5, alpha=0.6)
        
        # 3. Tracer la trajectoire dense
        dense_xs = [p['x'] for p in dense_trajectory]
        dense_ys = [p['y'] for p in dense_trajectory]
        
        ax.plot(dense_xs, dense_ys, color='magenta', linewidth=2.5, zorder=10, label='Itinéraire Dense')
        
        # Marquer le début et la fin
        ax.scatter([dense_xs[0]], [dense_ys[0]], color='red', s=100, zorder=11, label='Départ')
        ax.scatter([dense_xs[-1]], [dense_ys[-1]], color='purple', s=100, marker='X', zorder=11, label='Arrivée')

        ax.set_title("Visualisation de l'Itinéraire Dense (Corrigé)")
        ax.legend()
        plt.grid(True, linestyle='--', alpha=0.3)
        plt.show()