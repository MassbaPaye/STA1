/********************************************************/
/* Serveur TCP multi-voitures avec interface           */
/* Date : 14/10/2025                                   */
/* Version : 3.1                                       */
/********************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include "TCP_controleur.h"
#include "messages.h"
#include "message_controleur.h"

#define CHECK_ERROR(val1, val2, msg) if (val1==val2) { perror(msg); exit(EXIT_FAILURE); }

// Thread pour chaque voiture : réception des messages
void* receive_thread(void* arg) {
    VoitureConnection* v = (VoitureConnection*) arg;
    MessageType type;
    char buffer[2048];

    printf("[Serveur] Thread de la voiture %d démarré.\n", v->id_voiture);

    while (1) {
        int nbytes = recvMessage(v->sockfd, &type, buffer);
        if (nbytes <= 0) {
            printf("[Serveur] Connexion voiture %d fermée.\n", v->id_voiture);
            close(v->sockfd);
            break;
        }

        switch(type) {
            case MESSAGE_POSITION: {
                PositionVoiture* pos = (PositionVoiture*) buffer;
                printf("[Voiture %d] Position : (%d,%d,%d) theta=%.2f\n",
                       pos->id_voiture, pos->x, pos->y, pos->z, pos->theta);
                break;
            }

            case MESSAGE_DEMANDE: {
                Demande* d = (Demande*) buffer;
                printf("[Voiture %d] Demande : structure=%d type=%d dir=%c\n",
                       d->id_voiture, d->structure_id, d->type_operation, d->direction);
                break;
            }

            default:
                printf("[Voiture %d] Message type %d ignoré.\n", v->id_voiture, type);
                break;
        }
    }

    // Retirer la voiture de la liste
    pthread_mutex_lock(&mutex_voitures);
    for (int i = 0; i < nb_voitures; i++) {
        if (voitures_list[i].id_voiture == v->id_voiture) {
            for (int j = i; j < nb_voitures - 1; j++) {
                voitures_list[j] = voitures_list[j + 1];
            }
            nb_voitures--;
            break;
        }
    }
    pthread_mutex_unlock(&mutex_voitures);

    free(v);
    return NULL;
}

// Boucle interactive pour envoyer consignes/itinéraires
void* boucle_interactive(void* arg) {
    char cmd[32];
    int id;

    while (1) {
        printf("\nTapez 'consigne', 'itineraire', 'liste', ou 'fin' > ");
        (void)fgets(cmd, sizeof(cmd), stdin);
        cmd[strlen(cmd)-1] = '\0'; // supprimer le '\n'

        if (strcmp(cmd, "fin") == 0) break;

        if (strcmp(cmd, "liste") == 0) {
            pthread_mutex_lock(&mutex_voitures);
            printf("Voitures connectées : ");
            for (int i = 0; i < nb_voitures; i++) {
                printf("%d ", voitures_list[i].id_voiture);
            }
            printf("\n");
            pthread_mutex_unlock(&mutex_voitures);
            continue;
        }

        printf("ID de la voiture > ");
        (void)scanf("%d", &id);
        getchar(); // consommer le '\n' restant

        int sd = trouver_socket(id);
        if (sd < 0) {
            printf("Voiture %d non connectée.\n", id);
            continue;
        }

        if (strcmp(cmd, "consigne") == 0) {
            Consigne c = {1, 2, AUTORISATION};  // exemple
            sendMessage(sd, MESSAGE_CONSIGNE, &c);
            printf("[Serveur] Consigne envoyée à la voiture %d\n", id);
        } 
        else if (strcmp(cmd, "itineraire") == 0) {
            Itineraire iti;
            iti.id_voiture = id;
            iti.nb_points = 2;
            iti.points = malloc(sizeof(Point) * iti.nb_points);
            iti.points[0] = (Point){0,0,0,0};
            iti.points[1] = (Point){100,100,0,45.0f};
            sendMessage(sd, MESSAGE_ITINERAIRE, &iti);
            free(iti.points);
            printf("[Serveur] Itinéraire envoyé à la voiture %d\n", id);
        } 
        else {
            printf("Commande inconnue.\n");
        }
    }

    return NULL;
}

void* main_communication_controleur(void* arg) {
    int se;
    struct sockaddr_in adrlect;
    CHECK_ERROR((se = socket(AF_INET, SOCK_STREAM, 0)), -1, "Erreur création socket");

    adrlect.sin_family = AF_INET;
    adrlect.sin_port = htons(TCP_PORT);
    adrlect.sin_addr.s_addr = INADDR_ANY;

    CHECK_ERROR(bind(se, (struct sockaddr*)&adrlect, sizeof(adrlect)), -1, "Erreur bind");
    listen(se, 8);

    printf("[Serveur] En attente de connexion...\n");

    // Thread pour la boucle interactive
    pthread_t tid_ui;
    pthread_create(&tid_ui, NULL, boucle_interactive, NULL);

    while (1) {
        int client_sd = accept(se, NULL, NULL);
        CHECK_ERROR(client_sd, -1, "Erreur accept");

        // Créer une structure pour cette voiture
        VoitureConnection* v = malloc(sizeof(VoitureConnection));
        if (!v) { close(client_sd); continue; }

        v->sockfd = client_sd;
        v->id_voiture = nb_voitures + 1; // simple id pour l'exemple

        pthread_mutex_lock(&mutex_voitures);
        if (nb_voitures < MAX_VOITURES) {
            voitures_list[nb_voitures++] = *v;
        } else {
            printf("[Serveur] Nombre max de voitures atteint.\n");
            close(client_sd);
            free(v);
            pthread_mutex_unlock(&mutex_voitures);
            continue;
        }
        pthread_mutex_unlock(&mutex_voitures);

        pthread_create(&v->tid, NULL, receive_thread, v);

        printf("[Serveur] Nouvelle voiture connectée, id=%d\n", v->id_voiture);
    }

    close(se);
    pthread_join(tid_ui, NULL);
    return NULL;
}
