#include <stdint.h>
#include <pthread.h>   // pour pthread_mutex_t et pthread_t
#include "config.h"
#include "messages.h"

extern atomic_int voiture_connectee; 

/* === Fonctions génériques === */
int sendMessage(int sockfd, MessageType type, void* message);

/* === Fonctions spécialisées === */
int sendPositionVoiture(int sockfd, const PositionVoiture* pos);
int sendDemande(int sockfd, const Demande* dem);
int sendFin(int sockfd);

void deconnecter_controleur(pthread_t* tid_reception, int* sockfd_ptr);
void* initialisation_communication_voiture(void* arg);