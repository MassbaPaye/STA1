#ifndef TCP_VOITURE_H
#define TCP_VOITURE_H

#include <pthread.h>
#include <stdbool.h>
#include <netinet/in.h>
#include "messages.h"

extern int sockfd;         // socket associée à la voiture
extern pthread_t tid_rcv;      // thread qui gère cette connexion


// Thread de réception des messages du contrôleur   
void* receive_thread();

// Structure interne pour gérer la connexion
typedef struct {
    int sockfd;
    pthread_t tid_rcv;
    pthread_mutex_t mutex;
    bool voiture_connectee;
    bool stop_client;
    struct sockaddr_in serv_addr;
} ConnexionTCP;

extern ConnexionTCP connexion_tcp;

int recvConsigne(Consigne* cons);
int recvItineraire( Itineraire* iti);
int recvFin(char* buffer, size_t max_size);
int recvMessage(MessageType* type, void* message);

#endif