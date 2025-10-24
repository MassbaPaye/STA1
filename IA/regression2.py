import numpy as np
import matplotlib.pyplot as plt
import warnings

# Données : on repart de zéro
x = np.array([3, 356, 851, 1100, 1246], dtype=float)
y = np.array([16, 20, 30, 40, 50], dtype=float)

# Régression polynomiale de degré 6 (moindres carrés)
# Note : avec 5 points, un polynôme de degré 4 peut déjà passer exactement par tous les points.
# Un degré 6 est donc sur-paramétré ; np.polyfit retournera une solution de moindres carrés
# (souvent quasi-exacte numériquement) mais potentiellement mal conditionnée.
warnings.filterwarnings("ignore", category=np.RankWarning)
coeffs = np.polyfit(x, y, 6)
poly = np.poly1d(coeffs)

# Affichage des coefficients (du degré 6 vers le terme constant)
print("Coefficients (degré 6 -> constant) :")
for i, c in enumerate(coeffs):
    deg = 6 - i
    print(f"a_{deg} = {c:.12e}")

# Affichage de l'équation lisible
terms = []
for i, c in enumerate(coeffs):
    deg = 6 - i
    if abs(c) < 1e-12:
        continue
    if deg == 0:
        terms.append(f"{c:.6g}")
    elif deg == 1:
        terms.append(f"({c:.6g}) * x")
    else:
        terms.append(f"({c:.6g}) * x^{deg}")
eq_str = " + ".join(terms) if terms else "0"
print("\nPolynôme degré 6 trouvé :")
print(f"y(x) = {eq_str}")

# Vérification de l'ajustement sur les points
y_fit = poly(x)
max_err = np.max(np.abs(y_fit - y))
print(f"\nErreur max sur les points donnés : {max_err:.6e}")
for xi, yi, yfi in zip(x, y, y_fit):
    print(f"x={xi:8.3f}  y={yi:8.3f}  y_fit={yfi:10.6f}  err={yfi-yi: .3e}")

# Tracé entre x=0 et x=3000
x_pred = np.linspace(0, 1400, 300)
y_pred = poly(x_pred)
plt.scatter(x, y, label='Points donnés', s=40)
plt.plot(x_pred, y_pred, label='Polynôme degré 6', linewidth=2)
plt.xlabel('x')
plt.ylabel('y')
plt.title('Interpolation / Régression polynomiale (degré 6)')
plt.xlim(0, 1400)
plt.legend()
plt.grid(True, alpha=0.3)
plt.show()
