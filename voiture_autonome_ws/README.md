# Workspace Voiture Autonome

## 1. Introduction

Ce workspace contient le code source pour le développement d’un système de voiture autonome et de son contrôleur central.  
Il est organisé pour permettre un développement modulaire et indépendant des deux processus principaux :

- **Voiture** : processus représentant le véhicule autonome.  
- **Contrôleur** : processus de supervision et de pilotage.  

Les paramètres du **système global** sont configurable via des codes communs au deux processus (types, variables et fonctions partagées entre les processus). Des **outils** externes peuvent également être gréffés au workspace.

Chaque processus est compilé indépendamment à partir de son code source et des sources communes. Le workspace facilite la compilation automatique et la gestion des dépendances entre modules.

## 2. Structure générale

Le workspace est organisé en trois types de modules :

1. **Tools** (`src/tools/*/`)  
   - Contient des bibliothèques utilitaires réutilisables.  
   - Chaque tool doit avoir son propre Makefile. (Voir celui de my_tool pour l'exemple)  
   - Les tools doivent rester indépendants de la voiture et du contrôleur, ce sont des outils.
   - Exemple : La structure de File `queue` contient un Makefile et la compilation produit un objet `.o` dans `build/tools/queue/`.

2. **Common** (`src/common/`)  
   - Contient des fonctions et des définitions partagées par tous les processus.
   - `messages.h` défini les types de données des messages échanger entre les processus via le protocol TCP.
   - `config.h` contient des constantes permettant de configurer le réseau, et de parametrer le système avant la compilation.
   - Les fichiers `.c` et `.h` sont compilés en objets dans `build/common/`.

3. **Processus spécifiques** (`src/voiture/*/` et `src/controleur/*/`)  
   - Chaque processus contient ses modules propres (par exemple `Localisation` pour la voiture ou `IHM` poour le controleur).  
   - Chaque module doit avoir son propre Makefile pour être intégré à la compilation (Suivre l'exemple de `Localisation`).  
   - Les fichiers `.c` et `.h` sont compilés en objets dans `build/<process>/`.

**Remarque** : Les modules spécifiques peuvent inclure les modules communs et tools.

## 3. Système de compilation

Le Makefile global gère automatiquement :

1. La compilation des **tools**.
2. La compilation des fichiers **common**.
3. La compilation des fichiers du processus cible (`voiture` ou `controleur`) et de ces tâches.  

Lorsqu’un développeur exécute `make voiture` ou `make controleur` :

- Le Makefile scanne **automatiquement** les dossiers de chaque module pour trouver les fichiers `.c` et `.h`.
- Pour les tools, il scanne `src/tools/*/`.
- Pour common, il scanne `src/common/`.
- Pour un processus, il scanne `src/<process>/*/`.
- Les fichiers `.c` sont compilés en objets `.o` dans `build/<destination>/`.
- Le linkage combine tous les objets (processus + tools + common) pour générer l’exécutable.

Chaque module a la responsabilité de son Makefile interne, les variables `INCLUDES` et `BUILD_DIR` sont fournies par le makefile du processus : il doit générer ses objets dans le dossier de build du module.

**Exemple :**  
Si un développeur ajoute un nouveau tool `my_tool`, il doit créer :
`src/tools/my_tool/Makefile`

qui définit la compilation des sources vers `build/tools/my_tool/`.

## 4. Désactivation de modules

- Le Makefile global permet de compiler un processus en désactivant certains modules avec la variable `DISABLE`.
- Exemple : 
`make voiture DISABLE="Comportement,SuiviTrajectoire"`

- Cela permet de tester rapidement une partie du processus sans compiler tous les modules.

## 5. Points importants pour les développeurs

- **Isolation des processus** : Voiture et Contrôleur n’ont aucune dépendance entre eux.  
- **Indépendance des tools** : Un tool doit pouvoir être utilisé par plusieurs projets sans dépendance spécifique.  
- **Responsabilité du module** : Chaque module doit fournir son propre Makefile ou suivre le format de compilation standard du workspace.  
- **Scans automatiques** : Tous les fichiers `.c` et `.h` présents dans les dossiers modules sont pris en compte automatiquement à la compilation.  
- **Compilation modulaire** : L’ordre de compilation est toujours : tools → common → modules du processus → linkage final.

## 6. Compilation manuelle

Les commandes principales :

- `make all` : compile tools, common, voiture et controleur.  
- `make voiture` : compile seulement la voiture et ses dépendances (tools et common).  
- `make controleur` : compile seulement le contrôleur et ses dépendances.  
- `make clean` : supprime tous les fichiers générés.

## 7. Linkage

- Les fichiers objets sont combinés automatiquement pour générer l’exécutable final.  
- Les objets inclus sont :  
  - Objets du processus (`build/voiture/` ou `build/controleur/`)  
  - Objets des tools (`build/tools/`)  
  - Objets communs (`build/common/`)  
- Le linkage se fait avec `gcc -Wall -O2 -pthread -lm`.

## 8. Dépendances

- GCC compatible C99 ou supérieur.  
- Make.  
- Bibliothèques POSIX : `pthread` et `math`.

## 9. Bonnes pratiques

- Compiler tools et common avant les modules spécifiques.  
- Chaque développeur doit créer et maintenir le Makefile de son module.  
- Respecter l’isolation entre voiture et contrôleur.  
- Éviter toute dépendance circulaire entre modules.

