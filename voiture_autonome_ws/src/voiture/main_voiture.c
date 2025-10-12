#include <stdio.h>
#include <stdlib.h>
#include "config.h"
#include "utils.h"
#include "voiture_globals.h"
#include "main_localisation.h"

int main(int argc, char *argv[]) {
    DEBUG_PRINT("Mode DEBUG activé");
    if (argc < 2) {
        fprintf(stderr, "Usage : %s <ID_VOITURE>\n", argv[0]);
        return 1;
    }
    int id_voiture = atoi(argv[1]);
    if (id_voiture < 0 || id_voiture >= MAX_VOITURES) {
        fprintf(stderr, "Erreur : ID_VOITURE invalide (0-%d)\n", MAX_VOITURES - 1);
        return 1;
    }
    printf("🚗 Démarrage du processus VOITURE #%d\n", id_voiture);
    printf("→ Port TCP : %d\n", TCP_PORT);
    printf("→ IP du contrôleur : %s\n", CONTROLEUR_IP);

    // Initialisation des variables globales
    init_voiture_globals();
    start_localisation("Démarrage de la localisation !\n");
    // Boucle principale (placeholder)
    printf("La voiture %d est prête.\n", id_voiture);
    getchar();
    return 0;
}
