import numpy as np
import matplotlib.pyplot as plt

def plateau_quadratic(d, d0=10, sigma=20):
    """Loi 1 : plateau + décroissance quadratique"""
    p = np.ones_like(d)
    mask = d > d0
    p[mask] = 1 - ((d[mask] - d0)/sigma)**2
    p = np.clip(p, 0, 1)
    return p

def plateau_inv_quad(d, d0=10, sigma=20):
    """Loi 2 : plateau + inverse quadratique (rapide)"""
    p = np.ones_like(d)
    mask = d > d0
    p[mask] = 1 / (1 + ((d[mask]-d0)/sigma)**2)
    return p

# distances pour le graphe
d = np.linspace(0, 200, 500)
d0 = 50
sigma = 50
p1 = plateau_quadratic(d, d0=d0, sigma=sigma)
p2 = plateau_inv_quad(d, d0=d0, sigma=sigma)

plt.figure(figsize=(6,4))
plt.plot(d, p1, label="Plateau + quadratique")
plt.plot(d, p2, label="Plateau + inverse quadratique")
plt.axvline(10, color='gray', linestyle='--', label='d0 (plateau)')
plt.xlabel("Distance à la couleur de référence")
plt.ylabel("Probabilité")
plt.title("Comparaison des lois plateau")
plt.legend()
plt.grid(True)
plt.show()


