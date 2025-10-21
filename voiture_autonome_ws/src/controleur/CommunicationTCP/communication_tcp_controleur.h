#include <stdint.h>
#include <pthread.h>  
#include "config.h"
#include "messages.h"

/* === Fonctions génériques === */
int sendMessage(int Id, MessageType type, void* message);

/* === Fonctions spécialisées === */
int sendConsigne(int Id, const Consigne* cons);
int sendItineraire(int Id, const Itineraire* iti);
int sendFin(int Id);

void afficher_voitures_connectees();

void* initialisation_communication_controleur(void* arg);
void* test_thread(void* arg);