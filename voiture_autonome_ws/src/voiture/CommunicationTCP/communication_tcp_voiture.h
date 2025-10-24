#include <stdint.h>
#include <pthread.h>
#include <stdbool.h>
#include "config.h"
#include "messages.h"

// Envoie de Message
int sendMessage(MessageType type, void* message);
int sendPositionVoiture(const PositionVoiture* pos);
int sendDemande(const Demande* dem);
int sendFin();

void* initialisation_communication_voiture(void* arg);
bool est_connectee();