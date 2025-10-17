#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "TCP_voiture.h"
#include "communication_tcp.h"

/* --- Fonctions utilitaires internes --- */
static int sendBuffer(int sockfd, const void* buffer, size_t size) {
    return send(sockfd, buffer, size, 0);
}

/* === Fonctions spécialisées === */

int sendPositionVoiture(int sockfd, const PositionVoiture* pos) {
    return sendBuffer(sockfd, pos, sizeof(PositionVoiture));
}

int sendDemande(int sockfd, const Demande* dem) {
    return sendBuffer(sockfd, dem, sizeof(Demande));
}

int sendFin(int sockfd) {
    const char* texte = "fin";
    size_t len = strlen(texte) + 1; // inclure le '\0'
    return sendBuffer(sockfd, texte, len);
}

/* === Fonctions génériques === */

int sendMessage(int sockfd, MessageType type, void* message) {
    // envoyer d’abord le type
    sendBuffer(sockfd, &type, sizeof(MessageType));

    switch (type) {
        case MESSAGE_POSITION:   return sendPositionVoiture(sockfd, (PositionVoiture*)message);
        case MESSAGE_DEMANDE:    return sendDemande(sockfd, (Demande*)message);
        case MESSAGE_FIN:         return sendFin(sockfd);
        default: return -1;
    }
}

// Déconnecte proprement le client voiture du contrôleur
void deconnecter_controleur(pthread_t* tid_reception, int* sockfd_ptr) {
    if (!sockfd_ptr) return;

    int sockfd_local = *sockfd_ptr;

    // Fermer la socket TCP
    close(sockfd_local);
    *sockfd_ptr = -1;

    // Attendre que le thread de réception se termine
    if (tid_reception) {
        pthread_join(*tid_reception, NULL);
    }

    printf("[Client] Déconnecté proprement du contrôleur.\n");
}

void* initialisation_communication_voiture(void* arg) {
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

    pthread_create(&tid_rcv, NULL, receive_thread, NULL);

    return NULL;
}
