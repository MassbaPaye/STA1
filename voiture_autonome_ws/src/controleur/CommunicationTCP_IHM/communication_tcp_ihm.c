#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "communication_tcp_ihm.h"
#include "messages.h"
#include "controleur_globals.h"
#include "controleur_routier.h"

#define PORT 5001
#define CHECK_ERROR(val1, val2, msg) if ((val1)==(val2)) { perror(msg); exit(EXIT_FAILURE); }

int sock = -1;  // Socket unique de dialogue

// ==========================================================
// ===============   ENVOI DE MESSAGES   =====================
// ==========================================================

static int sendBuffer(const void* buffer, size_t size) {
    return send(sock, buffer, size, 0);
}

int sendMessage_ihm(int voiture_id, MessageType type, const void* message, size_t size) {
    MessageHeader header = { voiture_id, type, size };

    // Envoi du header
    if (sendBuffer(&header, sizeof(header)) <= 0)
        return -1;

    // Envoi du payload
    if (message && size > 0) {
        if (sendBuffer(message, size) <= 0)
            return -1;
    }

    return 1;
}

int sendPosition_ihm(int voiture_id, const PositionVoiture* pos) {
    return sendMessage_ihm(voiture_id, MESSAGE_POSITION, pos, sizeof(PositionVoiture));
}

int sendFin_ihm(void) {
    const char* texte = "fin";
    return sendMessage_ihm(0, MESSAGE_FIN, texte, strlen(texte) + 1);
}

// ==========================================================
// ===============   RECEPTION DE MESSAGES   =================
// ==========================================================

static int recvBuffer(void* buffer, size_t size) {
    return recv(sock, buffer, size, 0);
}

int recvMessage_ihm(int* voiture_id, MessageType* type, void* buffer, size_t max_size) {
    MessageHeader header;
    if (recvBuffer(&header, sizeof(header)) <= 0)
        return -1;

    *voiture_id = header.voiture_id;
    *type = header.type;

    size_t to_read = header.payload_size;
    if (to_read > max_size)
        to_read = max_size;

    if (to_read > 0) {
        if (recvBuffer(buffer, to_read) <= 0)
            return -1;
    }

    return (int)header.payload_size;
}

int recvItineraire_ihm(int* voiture_id, Itineraire* iti) {
    MessageType type;
    return recvMessage_ihm(voiture_id, &type, iti, sizeof(Itineraire));
}

int recvFin_ihm(int* voiture_id, char* buffer, size_t max_size) {
    MessageType type;
    return recvMessage_ihm(voiture_id, &type, buffer, max_size);
}

// ==========================================================
// ===============   COMMUNICATION PRINCIPALE   ==============
// ==========================================================

void* communication_ihm(void* arg) {
    (void)arg;
    int voiture_id;
    MessageType type;
    char buffer[2048];
    Itineraire iti;
    PositionVoiture pos;

    printf("[Serveur] Communication avec l'IHM démarrée.\n");

    while (1) {
        int nbytes = recvMessage_ihm(&voiture_id, &type, buffer, sizeof(buffer));
        if (nbytes <= 0) {
            printf("[Serveur] Connexion interrompue.\n");
            break;
        }

        switch (type) {
            case MESSAGE_ITINERAIRE:
                if (nbytes >= sizeof(Itineraire)) {
                    memcpy(&iti, buffer, sizeof(Itineraire));
                    printf("[IHM] Itinéraire reçu pour voiture %d (%d points)\n",
                           voiture_id, iti.nb_points);
                    set_voiture_itineraire(voiture_id, &iti);
                    Itineraire iti2;
                    get_voiture_itineraire(voiture_id, &iti2);
                    printf("point 1 : \n
                            point 2 : \n
                            point 3 : \n", iti2->Points.x, iti2->Points.x)

                    // Répondre avec une position exemple
                    pos.x = 1000.0f;
                    pos.y = 2000.0f;
                    pos.z = 0.0f;
                    pos.theta = 0.0f;
                    pos.vx = 0.0f;
                    pos.vy = 0.0f;
                    pos.vz = 0.0f;
                    sendPosition_ihm(voiture_id, &pos);
                }
                break;

            case MESSAGE_FIN:
                buffer[sizeof(buffer) - 1] = '\0';
                printf("[IHM] MESSAGE_FIN reçu : %s\n", buffer);
                goto fin_connexion;

            default:
                printf("[Serveur] Message type %d inconnu.\n", type);
                break;
        }
    }

fin_connexion:
    close(sock);
    printf("[Serveur] Connexion fermée proprement.\n");
    return NULL;
}

// ==========================================================
// ===============   INITIALISATION SERVEUR   ================
// ==========================================================

void* initialisation_communication_ihm(void* arg) {
    (void)arg;
    int se;
    struct sockaddr_in addr;

    CHECK_ERROR((se = socket(AF_INET, SOCK_STREAM, 0)), -1, "Erreur création socket");

    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    CHECK_ERROR(bind(se, (struct sockaddr*)&addr, sizeof(addr)), -1, "Erreur bind");
    CHECK_ERROR(listen(se, 1), -1, "Erreur listen");

    printf("[Serveur] En attente de connexion IHM sur le port %d...\n", PORT);

    sock = accept(se, NULL, NULL);
    CHECK_ERROR(sock, -1, "Erreur accept");

    printf("[Serveur] Connexion établie avec l'IHM.\n");

    communication_ihm(NULL);

    close(se);
    return NULL;
}
