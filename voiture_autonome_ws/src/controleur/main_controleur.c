#include <stdio.h>
#include "config.h"
#include "logger.h"
#include "controleur_globals.h"
#include "queue.h"
#include "TCP_controleur.h"

#define TAG "main"

int main() {
    DBG(TAG, "Mode DEBUG activé");

    INFO(TAG, "🧠 Démarrage du processus CONTROLEUR");
    INFO(TAG, "→ Port TCP : %d", TCP_PORT);
    INFO(TAG, "→ Nombre max de voitures : %d", MAX_VOITURES);

    // Initialisation des variables globales
    init_controleur_globals();
    main_communication_controleur(NULL);
    // Exemple : afficher la taille de la queue
    INFO(TAG, "→ Taille initiale de la queue de demandes : %d", size_demande_queue());

    // Boucle principale (placeholder)
    INFO(TAG, "Le contrôleur est prêt à recevoir les connexions.");
    getchar();
    
    return 0;
}
