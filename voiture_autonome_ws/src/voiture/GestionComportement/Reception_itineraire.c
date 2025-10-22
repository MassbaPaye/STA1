#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include"utils.h"
#include"config.h"
#include"logger.h"
#include "messages.h"
#include "voiture_globals.h"

#define TAG "itineraire_loader"

int charger_itineraire_csv(const char* chemin_fichier) {
    FILE* fichier = fopen(chemin_fichier, "r");
    if (!fichier) {
        DBG(TAG, "Impossible d'ouvrir le fichier CSV : %s", chemin_fichier);
        return -1;
    }

    char ligne[256];
    int nb_points = 0;

    // Première passe : compter le nombre de lignes (points)
    while (fgets(ligne, sizeof(ligne), fichier)) {
        if (ligne[0] == '#' || strlen(ligne) < 3) continue; // ignorer les commentaires/lignes vides
        nb_points++;
    }

    if (nb_points == 0) {
        DBG(TAG, "Aucun point trouvé dans le fichier CSV");
        fclose(fichier);
        return -1;
    }

    // Allouer la mémoire pour les points
    Point* points = malloc(nb_points * sizeof(Point));
    if (!points) {
        DBG(TAG, "Erreur d'allocation mémoire pour %d points", nb_points);
        fclose(fichier);
        return -1;
    }

    // Deuxième passe : lecture des données
    rewind(fichier);
    int idx = 0;
    while (fgets(ligne, sizeof(ligne), fichier)) {
        if (ligne[0] == '#' || strlen(ligne) < 3) continue;

        Point p = {0};
        int id = 0;
        // Format attendu : x,y,z,theta
        if (sscanf(ligne, "%d,%d,%d,%d,%f", &id, &p.x, &p.y, &p.z, &p.theta) != 4) {
            DBG(TAG, "Ligne CSV invalide : %s", ligne);
            continue;
        }

        points[idx++] = p;
        if (idx >= nb_points) break;
    }
    fclose(fichier);

    // Construire l'itinéraire
    Itineraire iti;
    iti.nb_points = idx;
    iti.points = points;

    // Enregistrer dans la structure globale
    if (set_itineraire(&iti) != 0) {
        DBG(TAG, "Échec de set_itineraire()");
        free(points);
        return -1;
    }

    DBG(TAG, "Itinéraire chargé avec succès (%d points) depuis %s", iti.nb_points, chemin_fichier);
    return 0;
}

int reception_itineraire() {
    if (charger_itineraire_csv("STA1/calcul_itinéraire/itineraire_dense.csv") != 0) {
        printf("Erreur lors du chargement de l'itinéraire.\n");
        return -1;
    }

    Itineraire iti;
    if (get_itineraire(&iti) == 0) {
        printf("Itinéraire chargé : %d points\n", iti.nb_points);
    }

    return 0;
}
