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

    Itineraire iti;

    // Deuxième passe : lecture des données
    rewind(fichier);
    int idx = 0;
    while (fgets(ligne, sizeof(ligne), fichier)) {
        if (ligne[0] == '#' || strlen(ligne) < 3) continue;

        Point p = {0};
        int id = 0;
        // Format attendu : x,y,z,theta
        if (sscanf(ligne, "%d,%f,%f,%f,%f", &id, &p.x, &p.y, &p.z, &p.theta) != 5) {
            DBG(TAG, "Ligne CSV invalide : %s", ligne);
            continue;
        }

        iti.points[idx++] = p;
        if (idx >= nb_points) break;
    }
    fclose(fichier);

    // Construire l'itinéraire
    iti.nb_points = idx;

    // Enregistrer dans la structure globale
    if (set_itineraire(&iti) != 0) {
        DBG(TAG, "Échec de set_itineraire()");
        return -1;
    }

    DBG(TAG, "Itinéraire chargé avec succès (%d points) depuis %s", iti.nb_points, chemin_fichier);
    return 0;
}

int reception_itineraire() {
    if (charger_itineraire_csv("itineraire_dense.csv") != 0) {
        printf("Erreur lors du chargement de l'itinéraire.\n");
        return -1;
    }

    Itineraire iti;
    if (get_itineraire(&iti) == 0) {
        printf("Itinéraire chargé : %d points\n", iti.nb_points);
    }

    return 0;
}
