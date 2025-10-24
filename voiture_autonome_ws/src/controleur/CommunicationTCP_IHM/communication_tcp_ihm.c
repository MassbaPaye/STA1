#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "communication_tcp_ihm.h"
#include "messages.h"
#include "controleur_globals.h"
#include "logger.h"

#define PORT 5001
#define CHECK_ERROR(val1, val2, msg) if ((val1)==(val2)) { perror(msg); exit(EXIT_FAILURE); }
#define TAG "Communication TCP IHM"

int sock = -1;

static int sendBuffer(const void* buffer, size_t size) {
    size_t total = 0;
    while (total < size) {
        ssize_t n = send(sock, (const char*)buffer + total, size - total, 0);
        if (n <= 0) return -1;
        total += n;
    }
    return (int)total;
}

int sendMessage_ihm( int id,  const PositionVoiture* pos) {
    MsgPos msg;
    msg.pos = *pos;
    msg.voiture_id = id;
    return sendBuffer( (const void*) &msg, sizeof(MsgIti));
}


int parse_msg_iti(const void* buffer, size_t buf_size, MsgIti* msg) {
    if (!buffer || !msg || buf_size < 8)  // minimum voiture_id + nb_points
        return -1;

    const char* ptr = (const char*)buffer;
    size_t offset = 0;

    memcpy(&msg->voiture_id, ptr + offset, sizeof(int));
    offset += sizeof(int);

    memcpy(&msg->iti.nb_points, ptr + offset, sizeof(int));
    offset += sizeof(int);

    if (msg->iti.nb_points < 0 || msg->iti.nb_points > MAX_ITI)
        return -1;
    size_t points_size = msg->iti.nb_points * sizeof(Point);
    if (buf_size < offset + points_size)
        return -1;
    memcpy(msg->iti.points, ptr + offset, points_size);
    return 0;
}

static int recv_all(int sockfd, void* buffer, size_t size) {
    size_t total = 0;
    while (total < size) {
        ssize_t n = recv(sockfd, (char*)buffer + total, size - total, 0);
        if (n <= 0) return -1;
        total += n;
    }
    return (int)total;
}

int recvItineraire_ihm( MsgIti* msg) {
    void* buffer = malloc(sizeof(MsgIti));
    int n = recv_all(sock, buffer, sizeof(MsgIti));
    if (n < 0 || n > sizeof(MsgIti)){
        ERR(TAG, "nbr de octet reçu : n=%d, attendu : <%d\n",n, sizeof(MsgIti));
        return -1;
    }
    parse_msg_iti(buffer, n, msg);
    free(buffer);
    return 0;
}

void* communication_ihm(void* arg) {
    (void)arg;
    MsgIti msg;

    INFO(TAG, "Recption avec l'IHM démarrée.");

    while (1) {
        if (recvItineraire_ihm( &msg) <= 0) {
            ERR(TAG, " Itineraire illisible.");
            break;
        } else {
            if (set_voiture_itineraire(msg.voiture_id, &msg.iti) < 0){
                ERR(TAG, "set voiture itineraire échoué");
            }
            INFO(TAG, "Iti recu : id=%d, nb_points=%d", msg.voiture_id, msg.iti.nb_points);
        }
    }
    return NULL;
}

void* initialisation_communication_ihm(void* arg) {
    (void)arg;
    int se;
    struct sockaddr_in addr;

    CHECK_ERROR((se = socket(AF_INET, SOCK_STREAM, 0)), -1, "Erreur création socket");

    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    CHECK_ERROR(bind(se, (struct sockaddr*)&addr, sizeof(addr)), -1, "Erreur bind");
    CHECK_ERROR(listen(se, 1), -1, "Erreur listen");

    printf("[Serveur] En attente de connexion IHM sur le port %d...\n", PORT);

    sock = accept(se, NULL, NULL);
    CHECK_ERROR(sock, -1, "Erreur accept");

    printf("[Serveur] Connexion établie avec l'IHM.\n");

    pthread_t tid;
    if (pthread_create(&tid, NULL, communication_ihm, NULL) != 0) {
        perror("Erreur pthread_create lancer réception ihm");
        return NULL;
    }

    close(se);
    return NULL;
}
