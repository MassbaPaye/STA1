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
#include "logger.h"
#include "controleur_globals.h"
#include "controleur_routier.h"

/* --- Mutex global --- */
pthread_mutex_t mutex_voitures = PTHREAD_MUTEX_INITIALIZER;

/* --- Tableau global de pointeurs et compteur --- */
VoitureConnection* voitures_list[MAX_VOITURES];
int nb_voitures = 0;

/* === Fonctions utilitaires === */

/* --- Récupère un pointeur vers la structure VoitureConnection par son ID --- */
VoitureConnection* get_voiture_by_id(int id_voiture) {
    VoitureConnection* v = NULL;
    pthread_mutex_lock(&mutex_voitures);
    for (int i = 0; i < nb_voitures; i++) {
        if (voitures_list[i]->id_voiture == id_voiture) {
            v = voitures_list[i];
            break;
        }
    }
    pthread_mutex_unlock(&mutex_voitures);
    return v;
}

/* --- Récupère un pointeur vers la structure VoitureConnection par sa socket --- */
VoitureConnection* get_voiture_by_sockfd(int sockfd) {
    VoitureConnection* v = NULL;
    pthread_mutex_lock(&mutex_voitures);
    for (int i = 0; i < nb_voitures; i++) {
        if (voitures_list[i]->sockfd == sockfd) {
            v = voitures_list[i];
            break;
        }
    }
    pthread_mutex_unlock(&mutex_voitures);
    return v;
}

static int recvBuffer(int sockfd, void* buffer, size_t size) {
    return recv(sockfd, buffer, size, 0);
}

int recvPositionVoiture(int sockfd, PositionVoiture* pos) {
    return recvBuffer(sockfd, pos, sizeof(PositionVoiture));
}

int recvDemande(int sockfd, Demande* dem) {
    return recvBuffer(sockfd, dem, sizeof(Demande));
}

int recvFin(int sockfd, char* buffer, size_t max_size) {
    int n = recvBuffer(sockfd, buffer, max_size);
    if (n <= 0) return n;
    buffer[max_size - 1] = '\0'; // sécurité
    return n;
}

int recvMessage(int sockfd, MessageType* type, void* message) {
    if (recvBuffer(sockfd, type, sizeof(MessageType)) <= 0) return -1;

    switch (*type) {
        case MESSAGE_POSITION:    return recvPositionVoiture(sockfd, (PositionVoiture*)message);
        case MESSAGE_DEMANDE:     return recvDemande(sockfd, (Demande*)message);
        case MESSAGE_FIN:         return recvFin(sockfd, (char*)message, 2048);
        default:                  return -1;
    }
}

// Thread pour chaque voiture : réception des messages
void* receive_thread(void* arg) {
    VoitureConnection* v = (VoitureConnection*) arg;
    MessageType type;
    char buffer[2048]; // pour MESSAGE_FIN si besoin

    printf("[Serveur] Thread de la voiture %d démarré.\n", v->id_voiture);

    while (1) {
        int nbytes = recvMessage(v->sockfd, &type, buffer);
        if (nbytes <= 0) {
            printf("[Serveur] Connexion voiture %d fermée (recv erreur).\n", v->id_voiture);
            break;
        }

        switch(type) {
            case MESSAGE_POSITION: {
                PositionVoiture* pos = (PositionVoiture*) buffer;
                set_voiture_position(v->id_voiture, pos, true);
                break;
            }
            case MESSAGE_DEMANDE: {
                Demande* d = (Demande*) buffer;
                enqueue_demande(d);
                gerer_demande(v->id_voiture);
                break;
            }
            case MESSAGE_FIN: {
                printf("[Voiture i] a envoyé MESSAGE_FIN : %s\n", buffer);
                goto fin_connexion; // sortir de la boucle et nettoyer
            }
            default:
                printf("[Voiture i] Message type %d ignoré.\n", type);
                break;
        }
    }

fin_connexion:
    // nettoyage
    deconnecter_voiture(v);

    return NULL;
}

// Déconnecte une voiture (ferme socket, enlève de la liste, libère mémoire)
void deconnecter_voiture(VoitureConnection* v) {
    if (!v) return;

    printf("[Serveur] Déconnexion de la voiture %d...\n", v->id_voiture);

    close(v->sockfd);

    pthread_mutex_lock(&mutex_voitures);
    for (int i = 0; i < nb_voitures; i++) {
        if (voitures_list[i] == v) {
            for (int j = i; j < nb_voitures - 1; j++) {
                voitures_list[j] = voitures_list[j + 1];
            }
            nb_voitures--;
            break;
        }
    }
    pthread_mutex_unlock(&mutex_voitures);

    free(v);
}

/* === Boucle interactive === */
/*
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
*/