#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "messages.h"
#include "logger.h"
#include "communication_tcp_controleur.h"
#include "TCP_controleur_voiture.h"
#include "controleur_globals.h"

#define TAG "TCP_controleur"
#define CHECK_ERROR(val1, val2, msg) if (val1==val2) { perror(msg); exit(EXIT_FAILURE); }

static int sendBuffer(int Id, const void* buffer, size_t size) {
    VoitureConnection* v = get_voiture_by_id(Id);
    if (!v) return -1;
    return send(v->sockfd, buffer, size, 0);
}

int sendConsigne(int Id, const Consigne* cons) {
    return sendBuffer(Id, cons, sizeof(Consigne));
}

int sendItineraire(int Id, const Itineraire* iti) {
    return sendBuffer(Id, iti, sizeof(Itineraire));
}

int sendFin(int Id) {
    const char* texte = "fin";
    size_t len = strlen(texte) + 1; // inclure le '\0'
    return sendBuffer(Id, texte, len);
}

int sendMessage(int Id, MessageType type, const void* message) {
    int a = sendBuffer(Id, &type, sizeof(MessageType));
    if (a <= 0) {
        ERR(TAG, "erreur a l'envoie du type");
        return a;
    }
    int b;
    switch (type) {
        case MESSAGE_CONSIGNE:    return sendConsigne(Id, (Consigne*)message);
        case MESSAGE_ITINERAIRE:  return sendItineraire(Id, (Itineraire*)message);
        case MESSAGE_FIN:         return sendFin(Id);
        default:                  return -1;
    }

}

// Affiche la liste des voitures connectées
void afficher_voitures_connectees() {
    pthread_mutex_lock(&mutex_voitures);

    if (nb_voitures == 0) {
        printf("Aucune voiture connectée.\n");
    } else {
        printf("Voitures connectées : ");
        for (int i = 0; i < nb_voitures; i++) {
            printf("%d ", voitures_list[i]->id_voiture);
        }
        printf("\n");
    }

    pthread_mutex_unlock(&mutex_voitures);
}

// Boucle principale de communication
void* initialisation_communication_controleur(void* arg) {
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

    while (1) {
        int client_sd = accept(se, NULL, NULL);
        CHECK_ERROR(client_sd, -1, "Erreur accept");

        VoitureConnection* v = malloc(sizeof(VoitureConnection));
        if (!v) { 
            close(client_sd); 
            continue; 
        }

        v->sockfd = client_sd;
        v->id_voiture = nb_voitures + 1;

        pthread_mutex_lock(&mutex_voitures);
        if (nb_voitures < MAX_VOITURES) {
            voitures_list[nb_voitures++] = v;  // stocker le pointeur
        } else {
            printf("[Serveur] Nombre max de voitures atteint.\n");
            close(client_sd);
            free(v);
            continue;
        }

        pthread_mutex_unlock(&mutex_voitures);
        pthread_create(&v->tid, NULL, receive_thread, v);

        printf("[Serveur] Nouvelle voiture connectée, id=%d\n", v->id_voiture);
    }

    close(se);
    return NULL;
}