Environnement Python pour le projet STA

Ce dossier contient la configuration pour l'environnement Python utilisé dans le projet.
Il permet d'installer facilement toutes les dépendances Python nécessaires sans interférer avec le reste du système.

---

Versions Python

- Le projet a été testé avec Python 3.11.x.
- Il est recommandé d’utiliser la même version de Python sur toutes les machines pour éviter les problèmes de compatibilité.
- Vous pouvez installer différentes versions de Python avec pyenv ou l’installateur officiel de Python si nécessaire.

---

Création de l'environnement virtuel

1. Cloner le repo et se placer à la racine du projet :

git clone <url_du_repo>
cd STA1

2. Créer un environnement virtuel local :

python3 -m venv python_env/venv

Cette commande crée un dossier venv dans STA1/python_env avec Python isolé pour ce projet.

3. Activer l'environnement virtuel :

# Linux / macOS
source python_env/venv/bin/activate

# Windows PowerShell
python_env\venv\Scripts\Activate.ps1

# Windows CMD
python_env\venv\Scripts\activate.bat

4. Installer les dépendances du projet :

pip install -r python_env/requirements.txt

---

Utilisation

- Activez toujours l’environnement virtuel avant de lancer des scripts Python du projet.
- Pour désactiver l’environnement virtuel :

deactivate

---

Notes GitHub

- Ne jamais commiter le dossier venv/.
- Seul requirements.txt doit être versionné pour partager la configuration avec l’équipe.
