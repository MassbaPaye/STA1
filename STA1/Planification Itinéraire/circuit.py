import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
import networkx as nx
import json


def calcul_itin(fichier_json, localisation_voiture, key, output_file="itineraire.json"):
    # === Charger le fichier JSON ===
    with open(fichier_json, 'r', encoding='utf-8') as f:
        data_json = json.load(f)

    # Extraire x et y seulement
    points_raw = data_json[key]
    points_array = np.array([[p[1], p[2]] for p in points_raw])
    points = points_array

    # === Convertir en DataFrame pour plus de confort ===
    data = pd.DataFrame(points, columns=['x', 'y'])

    # === Tracer le circuit ===
    x = data['x']
    y = data['y']

    plt.figure(figsize=(6,6))
    plt.plot(x, y, '-o', label='Circuit')
    plt.plot([x.iloc[-1], x.iloc[0]], [y.iloc[-1], y.iloc[0]], '-o', color='orange', label='Boucle fermée')
    plt.xlabel('x')
    plt.ylabel('y')
    plt.title('Cliquez sur 2 points du circuit')
    plt.legend()
    plt.axis('equal')
    plt.grid(True)

    # Position du véhicule
    voiture_xy = np.array([localisation_voiture["x"] , localisation_voiture["y"] ])  # mm → m
    plt.scatter(voiture_xy[0], voiture_xy[1], color='blue', s=100, label='Position voiture')

    # === Sélection interactive de 2 points ===
    print("Cliquez sur la destination dans le graphique...")
    selected_points = np.array(plt.ginput(1))
    plt.show()

    # === Créer un graphe avec les points du circuit ===
    G = nx.Graph()
    for i, p in enumerate(points):
        G.add_node(i, pos=p)

    # === Connecter les points du circuit (boucle fermée) ===
    for i in range(len(points)):
        G.add_edge(i, (i+1) % len(points), weight=np.linalg.norm(points[i] - points[(i+1) % len(points)]))

    # === Fonction : trouver les 2 points les plus proches d’un point donné ===
    def two_closest_indices(p, pts):
        dists = np.linalg.norm(pts - p, axis=1)
        return np.argsort(dists)[:2]  # les deux indices les plus proches

    # === Ajouter les points sélectionnés et les relier au circuit ===
    indices_selected = []
    
    # Point de départ (voiture)
    idx_start = len(points)
    G.add_node(idx_start, pos=voiture_xy)
    for ci in two_closest_indices(voiture_xy, points):
        G.add_edge(idx_start, ci, weight=np.linalg.norm(voiture_xy - points[ci]))
    indices_selected.append(idx_start)

    # Point d'arrivée (destination)
    dest_xy = selected_points[0]
    idx_end = len(points) + 1
    G.add_node(idx_end, pos=dest_xy)
    for ci in two_closest_indices(dest_xy, points):
        G.add_edge(idx_end, ci, weight=np.linalg.norm(dest_xy - points[ci]))
    indices_selected.append(idx_end)

    # === Calcul du plus court chemin ===
    start, end = indices_selected
    shortest_path = nx.shortest_path(G, source=start, target=end, weight='weight')

    # === Visualisation ===
    pos = nx.get_node_attributes(G, 'pos')
    path_coords = np.array([pos[i] for i in shortest_path])

    # === Conversion du chemin en format MESSAGE_ITINERAIRE ===

    def calculer_theta(p1, p2):
        """Calcule l'angle (theta) entre deux points en degrés."""
        dx = p2[0] - p1[0]
        dy = p2[1] - p1[1]
        return np.degrees(np.arctan2(dy, dx))

    # Construire la liste des points du chemin
    message_itineraire = {
        "type": 4,  # MESSAGE_ITINERAIRE
        "points": []
    }

    for i in range(len(path_coords)):
        x_mm = int(path_coords[i][0] * 1000)  # conversion m → mm si nécessaire
        y_mm = int(path_coords[i][1] * 1000)
        z_mm = 0  # altitude inconnue → 0 par défaut
        if i < len(path_coords) - 1:
            theta = float(calculer_theta(path_coords[i], path_coords[i + 1]))
        else:
            theta = 0.0  # dernier point
        message_itineraire["points"].append({
            "x": x_mm,
            "y": y_mm,
            "z": z_mm,
            "theta": theta
        })

    # === Sauvegarde en JSON (ou envoi série, socket, etc.) ===
    output_file = "itineraire.json"
    with open(output_file, "w", encoding="utf-8") as f:
        json.dump(message_itineraire, f, indent=4)

    print("✅ MESSAGE_ITINERAIRE généré et enregistré dans", output_file)



    plt.figure(figsize=(6,6))
    nx.draw(G, pos, node_size=5, node_color='gray', edge_color='lightgray')
    plt.plot(path_coords[:,0], path_coords[:,1], '-r', linewidth=2, label='Chemin le plus court')
    plt.scatter(selected_points[:,0], selected_points[:,1], color='green', s=80, zorder=3, label='Points sélectionnés')
    plt.title('Itinéraire le plus court entre les deux points')
    plt.legend()
    plt.axis('equal')
    plt.show()

    # === Résumé console ===
    print("🔹 Chemin (indices des noeuds) :", shortest_path)
    print("🔹 Longueur totale du chemin :", nx.path_weight(G, shortest_path, weight='weight'))

    return message_itineraire

if __name__ == "__main__":
    position_voiture = {"theta":0, "x":1.299, "y":3.433, "z":-1.863}
    calcul_itin("Marvelmind_log_data.json", position_voiture, "15", output_file="itineraire.json")