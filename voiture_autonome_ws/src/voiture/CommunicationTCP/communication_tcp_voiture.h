#include <stdint.h>
#include <pthread.h>   // pour pthread_mutex_t et pthread_t
#include <stdbool.h>
#include "config.h"
#include "messages.h"

/* === Fonctions génériques === */
int sendMessage(MessageType type, void* message);

/* === Fonctions spécialisées === */
int sendPositionVoiture(const PositionVoiture* pos);
int sendDemande(const Demande* dem);
int sendFin();

void deconnecter_controleur();
void* initialisation_communication_voiture(void* arg);
bool est_connectee();