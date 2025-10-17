#ifndef TCP_VOITURE_H
#define TCP_VOITURE_H

#include "messages.h"

extern int sockfd;         // socket associée à la voiture
extern pthread_t tid_rcv;      // thread qui gère cette connexion

void* receive_thread(void* arg);

int recvConsigne(int sockfd, Consigne* cons);
int recvItineraire(int sockfd, Itineraire* iti);
int recvFin(int sockfd, char* buffer, size_t max_size);
int recvMessage(int sockfd, MessageType* type, void* message);

#endif