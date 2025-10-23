#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <stdatomic.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "TCP_voiture.h"
#include "communication_tcp_voiture.h"
#include "logger.h"

#define TAG "communication_tcp_voiture"

atomic_int voiture_connectee = 0;

/* --- Fonctions utilitaires internes --- */
static int sendBuffer(const void* buffer, size_t size) {
    return send(connexion_tcp.sockfd, buffer, size, 0);
}

/* === Fonctions spécialisées === */

int sendPositionVoiture(const PositionVoiture* pos) {
    return sendBuffer(pos, sizeof(PositionVoiture));
}

int sendDemande(const Demande* dem) {
    return sendBuffer(dem, sizeof(Demande));
}

int sendFin() {
    const char* texte = "fin";
    size_t len = strlen(texte) + 1; // inclure le '\0'
    return sendBuffer( texte, len);
}

/* === Fonctions génériques === */

int sendMessage(MessageType type, void* message) {
    // envoyer d’abord le type
    sendBuffer(&type, sizeof(MessageType));

    switch (type) {
        case MESSAGE_POSITION:   return sendPositionVoiture( (PositionVoiture*)message);
        case MESSAGE_DEMANDE:    return sendDemande((Demande*)message);
        case MESSAGE_FIN:         return sendFin();
        default: return -1;
    }
}

// Déconnecte proprement le client voiture du contrôleur
void deconnecter_controleur() {
    pthread_mutex_lock(&connexion_tcp.mutex);
    connexion_tcp.stop_client = true;
    connexion_tcp.voiture_connectee = false;

    if (connexion_tcp.sockfd >= 0) {
        close(connexion_tcp.sockfd);
        connexion_tcp.sockfd = -1;
    }
    pthread_mutex_unlock(&connexion_tcp.mutex);

    // Attendre le thread de réception interne
    if (connexion_tcp.tid_rcv != 0) {
        pthread_cancel(connexion_tcp.tid_rcv);   // ou utiliser un flag d'arrêt dans receive_thread
        pthread_join(connexion_tcp.tid_rcv, NULL);
        connexion_tcp.tid_rcv = 0;
    }

    INFO(TAG, "Voiture déconnecté proprement du contrôleur");
}

void* initialisation_communication_voiture(void* arg) {
    INFO(TAG, "Thread communication tcp démarré");

    while (1) {
        pthread_mutex_lock(&connexion_tcp.mutex);
        bool stop = connexion_tcp.stop_client;
        bool connected = connexion_tcp.voiture_connectee;
        pthread_mutex_unlock(&connexion_tcp.mutex);

        if (stop) break;
        if (!connected) {
            // Création socket
            int sock = socket(AF_INET, SOCK_STREAM, 0);
            if (sock < 0) { perror("Erreur création socket"); sleep(5); continue; }

            connexion_tcp.serv_addr.sin_family = AF_INET;
            connexion_tcp.serv_addr.sin_port = htons(TCP_PORT);
            connexion_tcp.serv_addr.sin_addr.s_addr = inet_addr(CONTROLEUR_IP);

            if (connect(sock, (struct sockaddr*)&connexion_tcp.serv_addr, sizeof(connexion_tcp.serv_addr)) < 0) {
                perror("Erreur connexion");
                close(sock);
                sleep(5);
                continue;
            }

            pthread_mutex_lock(&connexion_tcp.mutex);
            connexion_tcp.sockfd = sock;
            connexion_tcp.voiture_connectee = true;
            // Lancement du thread de réception si pas déjà lancé
            if (connexion_tcp.tid_rcv == 0) {
                if (pthread_create(&connexion_tcp.tid_rcv, NULL, receive_thread, NULL) != 0) {
                    perror("Erreur création thread réception");
                    close(connexion_tcp.sockfd);
                    connexion_tcp.sockfd = -1;
                    connexion_tcp.voiture_connectee = false;
                    connexion_tcp.tid_rcv = 0;
                    pthread_mutex_unlock(&connexion_tcp.mutex);
                    sleep(5);
                    continue;
                }
            }
            pthread_mutex_unlock(&connexion_tcp.mutex);

            INFO(TAG, "Voiture Connectée au controleur");
        }

        usleep(100000); // 100 ms
    }

    // Fermeture propre à la sortie
    pthread_mutex_lock(&connexion_tcp.mutex);
    if (connexion_tcp.sockfd >= 0) { close(connexion_tcp.sockfd); connexion_tcp.sockfd = -1; }
    pthread_mutex_unlock(&connexion_tcp.mutex);

    return NULL;
}

bool est_connectee() {
    bool connected;

    pthread_mutex_lock(&connexion_tcp.mutex);
    connected = connexion_tcp.voiture_connectee;
    pthread_mutex_unlock(&connexion_tcp.mutex);

    return connected;
}
