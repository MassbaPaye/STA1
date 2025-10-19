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

#define TAG "communication TCP"

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

        int nbytes = recvMessage(sock, &type, buffer);

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
                printf("[Client] Consigne reçue : structure %d -> %s\n",
                       c->id_structure,
                       c->autorisation == CONSIGNE_AUTORISATION ? "AUTORISATION" : "ATTENTE");
                break;
            }

            case MESSAGE_ITINERAIRE: {
                Itineraire* iti = (Itineraire*) buffer;
                printf("[Client] Itinéraire reçu : %d points\n", iti->nb_points);
                for (int i = 0; i < iti->nb_points; i++) {
                    printf("  Point %d: (%d,%d,%d) theta=%.2f\n",
                           i, iti->points[i].x, iti->points[i].y, iti->points[i].z, iti->points[i].theta);
                }
                free(iti->points);
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


static int recvBuffer(int sockfd, void* buffer, size_t size) {
    return recv(sockfd, buffer, size, 0);
}

int recvConsigne(int sockfd, Consigne* cons) {
    return recvBuffer(sockfd, cons, sizeof(Consigne));
}

int recvItineraire(int sockfd, Itineraire* iti) {
    // réception du header
    recvBuffer(sockfd, &iti->nb_points, sizeof(int));
    // allocation dynamique des points
    iti->points = malloc(iti->nb_points * sizeof(Point));
    if (!iti->points) return -1;
    return recvBuffer(sockfd, iti->points, iti->nb_points * sizeof(Point));
}

int recvFin(int sockfd, char* buffer, size_t max_size) {
    int n = recvBuffer(sockfd, buffer, max_size);
    if (n <= 0) return n;
    buffer[max_size - 1] = '\0'; // sécurité
    return n;
}

int recvMessage(int sockfd, MessageType* type, void* message) {
    // lire le type
    int n = recvBuffer(sockfd, type, sizeof(MessageType));
    if (n <= 0) {
        perror("Erreur recvBuffer");
        return -1;  // ou gérer la déconnexion
    }
    INFO(TAG, "type : %d", *type);
    switch (*type) {
        case MESSAGE_CONSIGNE:   return recvConsigne(sockfd, (Consigne*)message);
        case MESSAGE_ITINERAIRE: return recvItineraire(sockfd, (Itineraire*)message);
        case MESSAGE_FIN:        return recvFin(sockfd, (char*)message, 2048);
        default: return -1;
    }
}