#include <stdio.h>
#include <stdlib.h>
#include "config.h"
#include "logger.h"
#include "communication_tcp_controleur.h"
#include "controleur_globals.h"
#include "queue.h"

#define TAG "main"

int main() {
    DBG(TAG, "Mode DEBUG activé");

    INFO(TAG, "🧠 Démarrage du processus CONTROLEUR");
    INFO(TAG, "→ Port TCP : %d", TCP_PORT);
    INFO(TAG, "→ Nombre max de voitures : %d", MAX_VOITURES);

    // Initialisation des variables globales
    init_controleur_globals();

    pthread_t thread_1;
    
    // Création du thread
    if (pthread_create(&thread_1, NULL, initialisation_communication_controleur, NULL) != 0) {
        perror("Erreur pthread_create");
        return EXIT_FAILURE;
    }

    // Exemple : afficher la taille de la queue
    INFO(TAG, "→ Taille initiale de la queue de demandes : %d", size_demande_queue());

    // Boucle principale (placeholder)
    INFO(TAG, "Le contrôleur est prêt à recevoir les connexions.");
    getchar();
    
    return 0;
}
