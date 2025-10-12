#include <stdio.h>
#include "config.h"
#include "utils.h"
#include "controleur_globals.h"
#include "queue.h"

int main() {
    DEBUG_PRINT("Mode DEBUG activé");

    printf("🧠 Démarrage du processus CONTROLEUR\n");
    printf("→ Port TCP : %d\n", TCP_PORT);
    printf("→ Nombre max de voitures : %d\n", MAX_VOITURES);

    // Initialisation des variables globales
    init_controleur_globals();

    // Exemple : afficher la taille de la queue
    printf("→ Taille initiale de la queue de demandes : %d\n", size_demande_queue());

    // Boucle principale (placeholder)
    printf("Le contrôleur est prêt à recevoir les connexions.\n");
    getchar();
    return 0;
}
