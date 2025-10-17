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
#include "message_voiture.h"
#include "config.h"
#include "TCP_voiture.h"

int sockfd; // socket globale

void* receive_thread(void* arg) {
    (void)arg;

    MessageType type;
    char buffer[2048];

    while (1) {
        int nbytes = recvMessage(sockfd, &type, buffer);
        if (nbytes <= 0) {
            printf("[Client] Connexion fermée ou erreur.\n");
            break;
        }

        switch (type) {
            case MESSAGE_CONSIGNE: {
                Consigne* c = (Consigne*) buffer;
                printf("[Client] Consigne reçue : voiture %d structure %d -> %s\n",
                       c->id_voiture, c->id_structure,
                       c->autorisation == AUTORISATION ? "AUTORISATION" : "ATTENTE");
                break;
            }

            case MESSAGE_ITINERAIRE: {
                Itineraire* iti = (Itineraire*) buffer;
                printf("[Client] Itinéraire reçu : voiture %d, %d points\n",
                       iti->id_voiture, iti->nb_points);
                for (int i = 0; i < iti->nb_points; i++) {
                    printf("  Point %d: (%d,%d,%d) theta=%.2f\n",
                           i, iti->points[i].x, iti->points[i].y, iti->points[i].z, iti->points[i].theta);
                }
                free(iti->points); // Important : recvItineraire alloue dynamiquement
                break;
            }

            default:
                printf("[Client] Message type %d ignoré.\n", type);
                break;
        }
    }

    return NULL;
}

void* main_communication_voiture(void* arg) {
    struct sockaddr_in serv_addr;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) { perror("Erreur création socket"); exit(1); }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(TCP_PORT);
    serv_addr.sin_addr.s_addr = inet_addr(CONTROLEUR_IP);

    if (connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Erreur connexion");
        exit(1);
    }

    printf("[Client] Connecté au serveur\n");

    pthread_t tid;
    pthread_create(&tid, NULL, receive_thread, NULL);

    // Boucle principale pour envoyer Position ou Demande
    char cmd[32];
    while (1) {
        printf("Tapez 'pos', 'dem', ou 'fin' > ");
        fgets(cmd, sizeof(cmd), stdin);
        cmd[strlen(cmd)-1] = '\0';

        if (strcmp(cmd, "fin") == 0) break;

        if (strcmp(cmd, "pos") == 0) {
            PositionVoiture pos = {1, 100, 200, 0, 90.0f, 10.0f, 0.0f, 0.0f};
            sendMessage(sockfd, MESSAGE_POSITION, &pos);
            printf("[Client] Position envoyée\n");
        } else if (strcmp(cmd, "dem") == 0) {
            Demande d = {1, 42, RESERVATION_STRUCTURE, 'n'};
            sendMessage(sockfd, MESSAGE_DEMANDE, &d);
            printf("[Client] Demande envoyée\n");
        }
    }

    close(sockfd);
    pthread_join(tid, NULL);
    return NULL;
}
