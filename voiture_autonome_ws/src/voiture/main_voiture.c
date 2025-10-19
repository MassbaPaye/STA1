#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include "logger.h"
#include "config.h"
#include "voiture_globals.h"
#include "main_localisation.h"
#include "main_module_exemple.h"
#include "messages.h"
#include "communication_tcp.h"
#include "communication_serie.h"
#include "utils.h"
#include <bits/getopt_core.h> // pour éviter une erreur sur optarg 


#define TAG "main"

pthread_t thread_localisation;
pthread_t thread_communication_tcp;
pthread_t thread_communication_serie;


int main(int argc, char *argv[]) {
    
    DBG(TAG, "Mode DEBUG activé");
    INFO(TAG, "🚗 Démarrage du processus VOITURE");
    INFO(TAG, "→ Port TCP : %d", TCP_PORT);
    INFO(TAG, "→ IP du contrôleur : %s", CONTROLEUR_IP);

    // Gestion des arguments à faire
    gestion_arguments(argc, argv);
    
    // Initialisation des variables globales
    init_voiture_globals();

    // Lancement de la localisation
    if (pthread_create(&thread_localisation, NULL, start_localisation, "Démarrage de la localisation !\n") != 0) {
        perror("Erreur pthread_create localisation");
        return EXIT_FAILURE;
    }

    // Lancement de la communication TCP avec le contrôleur routier
    if (pthread_create(&thread_communication_tcp, NULL, initialisation_communication_voiture, NULL) != 0) {
        perror("Erreur pthread_create init communication");
        return EXIT_FAILURE;
    }

    // Lancement de la communication Série avec le MegaPi
    if (pthread_create(&thread_communication_serie, NULL, lancer_communication_serie, NULL) != 0) {
        perror("Erreur pthread_create init communication");
        return EXIT_FAILURE;
    }
    // Attente de la connexion
    printf("En attente de la connexion avec le contrôleur...\n");
    while (!est_connectee()) {
        sleep(1);
    }
    INFO(TAG, "Communication TCP établie\n");



    exemple_module("Message du module d'exemple");
    
    INFO(TAG, "res=%d\n", affiche_position_actuelle());
    getchar();
    deconnecter_controleur();
    return 0;
}


