#include <stdint.h>
#include <pthread.h>  
#include "config.h"
#include "messages.h"

// Envoie de message
int sendMessage(int Id, MessageType type, const void* message);
int sendConsigne(int Id, const Consigne* cons);
int sendItineraire(int Id, const Itineraire* iti);
int sendFin(int Id);

void afficher_voitures_connectees();

void* initialisation_communication_controleur(void* arg);