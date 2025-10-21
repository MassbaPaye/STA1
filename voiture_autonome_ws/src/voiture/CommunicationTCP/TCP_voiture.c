/********************************************************/
/* Client TCP Voiture asynchrone                        */
/* Date : 14/10/2025                                    */
/* Version : 1.4                                        */
/********************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include "messages.h"
#include "config.h"
#include "TCP_voiture.h"
#include "logger.h"
#include "voiture_globals.h"

#define TAG "communication_tcp_voiture"

int sockfd = -1;
pthread_t tid_rcv = -1;

ConnexionTCP connexion_tcp = {
    .sockfd = -1,
    .tid_rcv = 0,
    .mutex = PTHREAD_MUTEX_INITIALIZER,
    .voiture_connectee = false,
    .stop_client = false
};

void* receive_thread() {
    MessageType type;
    char buffer[2048];

    while (1) {
        pthread_mutex_lock(&connexion_tcp.mutex);
        bool stop = connexion_tcp.stop_client;
        int sock = connexion_tcp.sockfd;
        pthread_mutex_unlock(&connexion_tcp.mutex);

        if (stop || sock < 0) break;

        int nbytes = recvMessage(&type, buffer);

        if (nbytes <= 0) {
            printf("[Client] Connexion fermée ou erreur.\n");

            // Notifier le thread principal
            pthread_mutex_lock(&connexion_tcp.mutex);
            connexion_tcp.voiture_connectee = false;
            pthread_mutex_unlock(&connexion_tcp.mutex);

            break;
        }

        // Traitement des messages (inchangé)
        switch (type) {
            case MESSAGE_CONSIGNE: {
                Consigne* c = (Consigne*) buffer;
                set_consigne(c);
                get_consigne(c);
                break;
            }

            case MESSAGE_ITINERAIRE: {
                Itineraire* iti = (Itineraire*) buffer;
                set_itineraire(iti);
                break;
            }

            case MESSAGE_FIN: {
                printf("[Client] Reçu MESSAGE_FIN, fermeture.\n");
                pthread_mutex_lock(&connexion_tcp.mutex);
                connexion_tcp.voiture_connectee = false;
                pthread_mutex_unlock(&connexion_tcp.mutex);
                return NULL;
            }

            default:
                printf("[Client] Message type %d ignoré.\n", type);
                break;
        }
    }

    return NULL;
}


static int recvBuffer(void* buffer, size_t size) {
    size_t total = 0;
    char* ptr = (char*) buffer;
    while (total < size) {
        ssize_t n = recv(connexion_tcp.sockfd, ptr + total, size - total, 0);
        if (n <= 0) return n; // erreur ou fermeture
        total += n;
    }
    return total;
}

int recvConsigne(Consigne* cons) {
    return recvBuffer(cons, sizeof(Consigne));
}

int recvItineraire(Itineraire* iti) {
    int n = recvBuffer(&iti->nb_points, sizeof(int));
    if (n <= 0) return n;

    iti->points = malloc(iti->nb_points * sizeof(Point));
    if (!iti->points) return -1;

    return recvBuffer(iti->points, iti->nb_points * sizeof(Point));
}

int recvFin(char* buffer, size_t max_size) {
    int n = recvBuffer(buffer, max_size);
    if (n <= 0) return n;
    buffer[max_size - 1] = '\0';
    return n;
}

int recvMessage(MessageType* type, void* message) {
    int n = recvBuffer(type, sizeof(MessageType));
    if (n <= 0) {
        perror("Erreur recvBuffer (type)");
        return -1;
    }

    int total = n;
    switch (*type) {
        case MESSAGE_CONSIGNE:   total += recvConsigne((Consigne*)message); break;
        case MESSAGE_ITINERAIRE: total += recvItineraire((Itineraire*)message); break;
        case MESSAGE_FIN:        total += recvFin((char*)message, 2048); break;
        default: return -1;
    }
    return total;
}