#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include "logger.h"
#include "config.h"
#include "voiture_globals.h"
#include "main_localisation.h"
#include "main_module_exemple.h"
#include "messages.h"
#include "communication_tcp.h"

#define TAG "main"

int main(int argc, char *argv[]) {
    #ifndef LOG_TO_FILE
    printf("LOG_TO_FILE");
    #else
    printf("not LOG_TO_FILE");
    #endif

    DBG(TAG, "Mode DEBUG activé");
    INFO(TAG, "Initialisation du système");
    if (argc < 2) {
        ERR(TAG, "Usage : %s <ID_VOITURE>\n", argv[0]);
        return 1;
    }
    int id_voiture = atoi(argv[1]);
    if (id_voiture < 0 || id_voiture >= MAX_VOITURES) {
        ERR(TAG, "Erreur : ID_VOITURE invalide (0-%d)\n", MAX_VOITURES - 1);
        return 1;
    }
    INFO(TAG, "🚗 Démarrage du processus VOITURE #%d", id_voiture);
    INFO(TAG, "→ Port TCP : %d", TCP_PORT);
    INFO(TAG, "→ IP du contrôleur : %s", CONTROLEUR_IP);
    
    // Initialisation des variables globales
    init_voiture_globals();
    start_localisation("Démarrage de la localisation !\n");

    pthread_t thread_1;

    // Création du thread
    if (pthread_create(&thread_1, NULL, initialisation_communication_voiture, NULL) != 0) {
        perror("Erreur pthread_create");
        return EXIT_FAILURE;
    }

    exemple_module("Message du module d'exemple");
    
    INFO(TAG, "res=%d\n", affiche_position_actuelle());
    
    // Boucle principale (placeholder)
    INFO(TAG, "La voiture %d est prête.\n", id_voiture);
    getchar();

    return 0;
}
