#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include "utils.h"
#include "logger.h"
#include "config.h"
#include "voiture_globals.h"
#include "main_localisation.h"
#include "main_module_exemple.h"
#include "messages.h"
#include "communication_tcp_voiture.h"
#include "communication_serie.h"
#include "simulation_loc.h"
#include "UDP_voiture.h"
#include "Gestion_comportement.h"

#define TAG "main"

pthread_t thread_localisation;
pthread_t thread_communication_tcp;
pthread_t thread_communication_udp;
pthread_t thread_communication_serie;
pthread_t thread_gestion_comportement;
#ifdef SIMULATION
pthread_t thread_simulation;
#endif

/*
void* thread_periodique(void* arg) {
    PositionVoiture pos;
    DemandeType a = RESERVATION_STRUCTURE;
    Demande d;
    int b = 0;
    while (1) {
        pos.x = b;
        pos.y = b;
        pos.z = b;
        pos.theta = b;
        pos.vx = b;
        pos.vy = b;
        pos.vz = b;
        d.structure_id = 1;
        d.type = a;
        sendMessage(MESSAGE_DEMANDE, &d);
        sendMessage(MESSAGE_POSITION, &pos);
        if (a == RESERVATION_STRUCTURE){
            a = LIBERATION_STRUCTURE;
        } else {
            a = RESERVATION_STRUCTURE;
        }
        b += 1;
        sleep(3); // attend 3 secondes
    }
    return NULL;
}
*/
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
    if (pthread_create(&thread_localisation, NULL, lancer_localisation_thread, NULL) != 0) {
        perror("Erreur pthread_create localisation");
        return EXIT_FAILURE;
    }

    // Lancement de la communication TCP avec le contrôleur
    if (pthread_create(&thread_communication_tcp, NULL, initialisation_communication_voiture, NULL) != 0) {
        perror("Erreur pthread_create init communication tcp");
        return EXIT_FAILURE;
    }

#if USE_SERIAL
    // Lancement de la communication Série avec le MegaPi
    if (pthread_create(&thread_communication_serie, NULL, lancer_communication_serie, NULL) != 0) {
        perror("Erreur pthread_create lancer communication serie");
        return EXIT_FAILURE;
    }
#else
    INFO(TAG, "Communication série désactivée (USE_SERIAL=0)");
#endif

    // Lancement du thread UDP
    if (pthread_create(&thread_communication_udp, NULL, initialisation_communication_camera, NULL) != 0) {
        perror("Erreur pthread_create lancer communication udp");
        return EXIT_FAILURE;
    } else {
        printf("Connexion UDP avec la caméra bien lancée\n");
    }

    // Attente de la connexion
    printf("En attente de la connexion avec le contrôleur...\n");
    while (!est_connectee()) {
        sleep(1);
    }
    INFO(TAG, "=============================");
    INFO(TAG, "= Communication TCP établie =");
    INFO(TAG, "=============================");
    
    // Lancement du thread comportement
    if (pthread_create(&thread_gestion_comportement, NULL, lancer_comportement, NULL) != 0) {
        perror("Erreur pthread_create lancer comportement");
        return EXIT_FAILURE;
    }

    #ifdef SIMULATION
    // Lancement de la simulation
    if (pthread_create(&thread_simulation, NULL, lancer_simulateur, NULL) != 0) {
        perror("Erreur pthread_create lancement simulation");
        return EXIT_FAILURE;
    }
    #endif
    /*
    pthread_t tid;
    if (pthread_create(&tid, NULL, thread_periodique, NULL) != 0) {
        perror("Erreur pthread_create lancer communication serie");
        return EXIT_FAILURE;
    }
    */
    /* // code module exemple
    exemple_module("Message du module d'exemple");
    INFO(TAG, "res=%d\n", affiche_position_actuelle());
    */
   
    // Attendre la fin du thread (ici il tourne en boucle infinie)
    pthread_join(thread_communication_udp, NULL);
    
    getchar();
<<<<<<< HEAD
    stop_comportement();
=======
#if USE_SERIAL
>>>>>>> b371a65 (Model Lourd IA Fonctionnel)
    stop_communication_serie();
#endif
    stop_localisation();
    deconnecter_controleur();
    return 0;
}

