#include "communication_tcp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "messages.h"

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

/* === Communication bas niveau === */
static int sendBuffer(int sockfd, const void* buffer, size_t size) {
    return send(sockfd, buffer, size, 0);
}

static int recvBuffer(int sockfd, void* buffer, size_t size) {
    return recv(sockfd, buffer, size, 0);
}

/* === Fonctions spécialisées === */
int sendConsigne(int sockfd, const Consigne* cons) {
    return sendBuffer(sockfd, cons, sizeof(Consigne));
}

int sendItineraire(int sockfd, const Itineraire* iti) {
    sendBuffer(sockfd, &iti->nb_points, sizeof(int));
    return sendBuffer(sockfd, iti->points, iti->nb_points * sizeof(Point));
}

int recvPositionVoiture(int sockfd, PositionVoiture* pos) {
    return recvBuffer(sockfd, pos, sizeof(PositionVoiture));
}

int recvDemande(int sockfd, Demande* dem) {
    return recvBuffer(sockfd, dem, sizeof(Demande));
}

int sendFin(int sockfd) {
    const char* texte = "fin";
    size_t len = strlen(texte) + 1; // inclure le '\0'
    return sendBuffer(sockfd, texte, len);
}

int recvFin(int sockfd, char* buffer, size_t max_size) {
    int n = recvBuffer(sockfd, buffer, max_size);
    if (n <= 0) return n;
    buffer[max_size - 1] = '\0'; // sécurité
    return n;
}

/* === Fonctions génériques === */
int sendMessage(int sockfd, MessageType type, void* message) {
    if (sendBuffer(sockfd, &type, sizeof(MessageType)) <= 0) return -1;

    switch (type) {
        case MESSAGE_CONSIGNE:    return sendConsigne(sockfd, (Consigne*)message);
        case MESSAGE_ITINERAIRE:  return sendItineraire(sockfd, (Itineraire*)message);
        case MESSAGE_FIN:         return sendFin(sockfd);
        default:                  return -1;
    }
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

/* Envoi par ID */
int sendMessageById(int id, MessageType type, void* message) {
    VoitureConnection* v = get_voiture_by_id(id);
    if (!v) return -1;
    return sendMessage(v->sockfd, type, message);
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
                printf("[Voiture] Position : (%d,%d,%d) theta=%.2f\n",
                       pos->x, pos->y, pos->z, pos->theta);
                break;
            }
            case MESSAGE_DEMANDE: {
                Demande* d = (Demande*) buffer;
                printf("[Voiture] Demande : structure=%d type=%d dir=%c\n",
                       d->structure_id, d->type_operation, d->direction);
                break;
            }
            case MESSAGE_FIN: {
                printf("[Voiture %d] a envoyé MESSAGE_FIN : %s\n", v->id_voiture, buffer);
                goto fin_connexion; // sortir de la boucle et nettoyer
            }
            default:
                printf("[Voiture %d] Message type %d ignoré.\n", v->id_voiture, type);
                break;
        }
    }

fin_connexion:
    // nettoyage
    deconnecter_voiture(v);

    return NULL;
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
