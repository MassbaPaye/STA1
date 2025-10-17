#include "message_controleur.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "messages.h"

/* --- Définition du mutex global --- */
pthread_mutex_t mutex_voitures = PTHREAD_MUTEX_INITIALIZER;

/* --- Définition du tableau de connexions et du compteur --- */
VoitureConnection voitures_list[MAX_VOITURES];
int nb_voitures = 0;

/* --- Fonctions utilitaires internes --- */
static int sendBuffer(int sockfd, const void* buffer, size_t size) {
    return send(sockfd, buffer, size, 0);
}

static int recvBuffer(int sockfd, void* buffer, size_t size) {
    return recv(sockfd, buffer, size, 0);
}

/* === Fonctions spécialisées === */

int sendPositionVoiture(int sockfd, const PositionVoiture* pos) {
    return sendBuffer(sockfd, pos, sizeof(PositionVoiture));
}

int sendDemande(int sockfd, const Demande* dem) {
    return sendBuffer(sockfd, dem, sizeof(Demande));
}

int sendConsigne(int sockfd, const Consigne* cons) {
    return sendBuffer(sockfd, cons, sizeof(Consigne));
}

int sendItineraire(int sockfd, const Itineraire* iti) {
    // envoi du header
    sendBuffer(sockfd, &iti->id_voiture, sizeof(int));
    sendBuffer(sockfd, &iti->nb_points, sizeof(int));
    // envoi des points
    return sendBuffer(sockfd, iti->points, iti->nb_points * sizeof(Point));
}

int recvPositionVoiture(int sockfd, PositionVoiture* pos) {
    return recvBuffer(sockfd, pos, sizeof(PositionVoiture));
}

int recvDemande(int sockfd, Demande* dem) {
    return recvBuffer(sockfd, dem, sizeof(Demande));
}

int recvConsigne(int sockfd, Consigne* cons) {
    return recvBuffer(sockfd, cons, sizeof(Consigne));
}

int recvItineraire(int sockfd, Itineraire* iti) {
    // réception du header
    recvBuffer(sockfd, &iti->id_voiture, sizeof(int));
    recvBuffer(sockfd, &iti->nb_points, sizeof(int));
    // allocation dynamique des points
    iti->points = malloc(iti->nb_points * sizeof(Point));
    if (!iti->points) return -1;
    return recvBuffer(sockfd, iti->points, iti->nb_points * sizeof(Point));
}

/* === Fonctions génériques === */

int sendMessage(int sockfd, MessageType type, void* message) {
    // envoyer d’abord le type
    sendBuffer(sockfd, &type, sizeof(MessageType));

    switch (type) {
        case MESSAGE_POSITION:   return sendPositionVoiture(sockfd, (PositionVoiture*)message);
        case MESSAGE_DEMANDE:    return sendDemande(sockfd, (Demande*)message);
        case MESSAGE_CONSIGNE:   return sendConsigne(sockfd, (Consigne*)message);
        case MESSAGE_ITINERAIRE: return sendItineraire(sockfd, (Itineraire*)message);
        default: return -1;
    }
}

int recvMessage(int sockfd, MessageType* type, void* message) {
    // lire le type
    recvBuffer(sockfd, type, sizeof(MessageType));

    switch (*type) {
        case MESSAGE_POSITION:   return recvPositionVoiture(sockfd, (PositionVoiture*)message);
        case MESSAGE_DEMANDE:    return recvDemande(sockfd, (Demande*)message);
        case MESSAGE_CONSIGNE:   return recvConsigne(sockfd, (Consigne*)message);
        case MESSAGE_ITINERAIRE: return recvItineraire(sockfd, (Itineraire*)message);
        default: return -1;
    }
}

// Recherche la socket d'une voiture par son id
int trouver_socket(int id_voiture) {
    int sd = -1;
    pthread_mutex_lock(&mutex_voitures);
    for (int i = 0; i < nb_voitures; i++) {
        if (voitures_list[i].id_voiture == id_voiture) {
            sd = voitures_list[i].sockfd;
            break;
        }
    }
    pthread_mutex_unlock(&mutex_voitures);
    return sd;
}