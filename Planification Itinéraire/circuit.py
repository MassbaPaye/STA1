import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
import networkx as nx

# === Charger le fichier CSV ===
fichier = "Classeur1.csv"
data = pd.read_csv(fichier, sep=';')
points = data[['x', 'y']].to_numpy()

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

# === Sélection interactive de 2 points ===
print("Cliquez sur deux points dans le graphique...")
selected_points = plt.ginput(2)
selected_points = np.array(selected_points)
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
for i, p in enumerate(selected_points):
    idx = len(points) + i
    G.add_node(idx, pos=p)
    closest_idxs = two_closest_indices(p, points)
    
    # Relier aux deux points les plus proches
    for ci in closest_idxs:
        G.add_edge(idx, ci, weight=np.linalg.norm(p - points[ci]))
    
    indices_selected.append(idx)

# === Calcul du plus court chemin ===
start, end = indices_selected
shortest_path = nx.shortest_path(G, source=start, target=end, weight='weight')

# === Visualisation ===
pos = nx.get_node_attributes(G, 'pos')
path_coords = np.array([pos[i] for i in shortest_path])

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
