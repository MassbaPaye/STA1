#ifndef COMMUNICATION_TCP_IHM_H
#define COMMUNICATION_TCP_IHM_H

#include <stddef.h>
#include "messages.h"

// Socket unique de dialogue
extern int sock;

// Header commun à tous les messages
typedef struct {
    int voiture_id;     // Identifiant de la voiture concernée
    MessageType type;   // Type du message (ITINERAIRE, POSITION, etc.)
    size_t payload_size;// Taille utile des données
} MessageHeader;

// === Envoi ===
int sendPosition_ihm(int voiture_id, const PositionVoiture* pos);
int sendFin_ihm(void);
int sendMessage_ihm(int voiture_id, MessageType type, const void* message, size_t size);

// === Réception ===
int recvMessage_ihm(int* voiture_id, MessageType* type, void* buffer, size_t max_size);
int recvItineraire_ihm(int* voiture_id, Itineraire* iti);
int recvFin_ihm(int* voiture_id, char* buffer, size_t max_size);

// Fonction principale de communication
void* communication_ihm(void* arg);

// Initialisation du serveur TCP
void* initialisation_communication_ihm(void* arg);

#endif
