# Projet Voiture Autonome – Workspace de Développement

## 1. Présentation

Ce projet constitue un workspace modulaire en langage C pour le développement d’un système de voiture autonome et de son contrôleur central.
L’architecture est conçue pour être claire, maintenable et adaptée au développement embarqué.
Chaque composant (voiture, contrôleur, outils, communs) est compilé séparément puis lié au sein d’un exécutable final.

## 2. Structure du workspace

build/ contient les objets compilés et les exécutables :
- voiture/ : objets et exécutable de la voiture
- controleur/ : objets et exécutable du contrôleur
- tools/ : objets des utilitaires

src/ contient le code source :
- common/ : fichiers partagés (headers et utilitaires)
- tools/ : modules outils
- voiture/ : code du véhicule autonome
- controleur/ : code du contrôleur

Le Makefile global est le point d’entrée principal. README.md contient la documentation.

## 3. Architecture logicielle

Le projet repose sur quatre ensembles principaux : tools, common, voiture et controleur.
Chaque module a son Makefile interne, invoqué automatiquement par le Makefile global.

## 4. Compilation

- `make all` compile tous les modules.
- `make voiture` compile uniquement la voiture. On peut désactiver certains modules via `DISABLE="Module1,Module2"`.
- `make controleur` compile uniquement le contrôleur.
- `make clean` supprime tous les fichiers générés.

Exécutables générés : `build/voiture/voiture` et `build/controleur/controleur`.

## 5. Aide intégrée

`make help` affiche la liste des cibles disponibles et leur description.
Si une commande inconnue est saisie, le système redirige automatiquement vers `make help`.

## 6. Processus de compilation

Pour la voiture : 
1. Compilation des outils (tools)
2. Compilation des fichiers communs (common)
3. Compilation des modules spécifiques à la voiture
4. Linkage de tous les fichiers objets pour créer l’exécutable

Le Makefile affiche les fichiers objets utilisés lors du linkage.

## 7. Détails techniques

- GCC avec options : -Wall -O2 -pthread -lm
- Includes détectés automatiquement depuis `src/tools/`
- Linkage après compilation de tous les objets nécessaires
- Ordre de compilation géré pour éviter les erreurs

## 8. Bonnes pratiques

- Exécuter `make clean` avant un build complet si des fichiers source ont été supprimés.
- Utiliser `DISABLE=` pour isoler un module et accélérer le développement.
- Centraliser les déclarations partagées dans `src/common/`.
- Minimiser les dépendances entre les modules voiture et controleur.

## 9. Dépendances externes

Outils standards :
- GCC
- Make
- Bibliothèques POSIX : pthread, math (-lpthread, -lm)

Aucune dépendance tierce nécessaire.

## 10. Auteur et maintenance

Développé et maintenu par Victor.
Dernière mise à jour : octobre 2025
Le workspace sert de base à des développements embarqués ou distribués.

## 11. Évolutions prévues

- Ajout d’un système de tests unitaires internes
- Extension des modules de communication (TCP, série)
- Intégration sur plateforme embarquée (Raspberry Pi, STM32)
- Amélioration du logging et de la supervision temps réel

## 12. Résumé

Workspace structuré pour un véhicule autonome en C, modulaire et extensible.
Organisation claire, facilite maintenance, tests et intégration continue.
