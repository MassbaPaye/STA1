/********************************************************/
/* Serveur TCP multi-voitures avec interface            */
/* Date : 14/10/2025                                    */
/* Version : 3.3                                        */
/********************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include "TCP_controleur.h"
#include "messages.h"
#include "communication_tcp.h"
#include "logger.h"

#define TAG "TCP_controleur"
#define CHECK_ERROR(val1, val2, msg) if (val1==val2) { perror(msg); exit(EXIT_FAILURE); }

/* === Boucle interactive === */
void* boucle_interactive(void* arg) {
    (void)arg;
    char cmd[32];
    int id;

    while (1) {
        printf("\nTapez 'consigne', 'itineraire', 'liste', ou 'fin' > ");
        if (!fgets(cmd, sizeof(cmd), stdin)) continue;  // ignore erreur
        cmd[strcspn(cmd, "\n")] = '\0';  // supprimer le '\n'

        if (strcmp(cmd, "liste") == 0) {
            afficher_voitures_connectees();
            continue;
        }

        printf("ID de la voiture > ");
        if (scanf("%d", &id) != 1) { getchar(); continue; }  // ignorer erreur
        getchar(); // consommer '\n' restant

        VoitureConnection* v = get_voiture_by_id(id);
        if (!v) {
            printf("Voiture %d non connectée.\n", id);
            continue;
        }
        int sd = v->sockfd;

        if (strcmp(cmd, "fin") == 0) {
            sendFin(sd);
            deconnecter_voiture(v);
            continue;
        }

        if (strcmp(cmd, "consigne") == 0) {
            Consigne c = {2, CONSIGNE_AUTORISATION};
            sendMessage(sd, MESSAGE_CONSIGNE, &c);
            INFO(TAG, "[Serveur] Consigne envoyée à la voiture %d\n", id);
        } 
        else if (strcmp(cmd, "itineraire") == 0) {
            Itineraire iti;
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

/* === Boucle principale de communication === */
void* main_communication_controleur(void* arg) {
    (void)arg;
    int se;
    struct sockaddr_in adrlect;
    CHECK_ERROR((se = socket(AF_INET, SOCK_STREAM, 0)), -1, "Erreur création socket");

    adrlect.sin_family = AF_INET;
    adrlect.sin_port = htons(TCP_PORT);
    adrlect.sin_addr.s_addr = INADDR_ANY;

    CHECK_ERROR(bind(se, (struct sockaddr*)&adrlect, sizeof(adrlect)), -1, "Erreur bind");
    listen(se, 8);

    printf("[Serveur] En attente de connexion...\n");

    pthread_t tid_ui;
    pthread_create(&tid_ui, NULL, boucle_interactive, NULL);

    while (1) {
        int client_sd = accept(se, NULL, NULL);
        CHECK_ERROR(client_sd, -1, "Erreur accept");

        VoitureConnection* v = malloc(sizeof(VoitureConnection));
        if (!v) { close(client_sd); continue; }

        v->sockfd = client_sd;
        v->id_voiture = nb_voitures + 1;

        pthread_mutex_lock(&mutex_voitures);
        if (nb_voitures < MAX_VOITURES) {
            voitures_list[nb_voitures++] = v;  // stocker le pointeur directement
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
