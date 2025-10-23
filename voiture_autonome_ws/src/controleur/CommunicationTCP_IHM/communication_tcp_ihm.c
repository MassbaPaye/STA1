#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "communication_tcp_ihm.h"
#include "messages.h"
#include "controleur_globals.h"

#define PORT 5001
#define CHECK_ERROR(val1, val2, msg) if ((val1)==(val2)) { perror(msg); exit(EXIT_FAILURE); }

int sock = -1;  // Socket unique de dialogue

// ==========================================================
// ===============   ENVOI DE MESSAGES   ==================
// ==========================================================

static int sendBuffer(const void* buffer, size_t size) {
    size_t total = 0;
    while (total < size) {
        ssize_t n = send(sock, (const char*)buffer + total, size - total, 0);
        if (n <= 0) return -1;
        total += n;
    }
    return (int)total;
}

int sendMessage_ihm(int voiture_id, MessageType type, const void* message, size_t size) {
    MessageHeader header = { voiture_id, type, size };

    if (sendBuffer(&header, sizeof(header)) <= 0)
        return -1;

    if (message && size > 0) {
        if (sendBuffer(message, size) <= 0)
            return -1;
    }

    return 1;
}

// ==========================================================
// ===============   RECEPTION DE MESSAGES   ==============
// ==========================================================

static int recvBuffer(void* buffer, size_t size) {
    size_t total = 0;
    while (total < size) {
        ssize_t n = recv(sock, (char*)buffer + total, size - total, 0);
        if (n <= 0) return -1;
        total += n;
    }
    return (int)total;
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

// ==========================================================
// ===============   RECEPTION D’UN ITINERAIRE   ==========
// ==========================================================

int recvItineraire_ihm(int* voiture_id, Itineraire* iti) {
    MessageType type;
    
    // Lecture du header + payload dans un buffer temporaire dynamique
    char header_buf[sizeof(int)]; // pour lire nb_points
    if (recvMessage_ihm(voiture_id, &type, header_buf, sizeof(header_buf)) <= 0)
        return -1;

    if (type != MESSAGE_ITINERAIRE)
        return -1;

    // Lecture de nb_points
    memcpy(&iti->nb_points, header_buf, sizeof(int));
    if (iti->nb_points <= 0) {
        fprintf(stderr, "[Erreur] nb_points invalide: %d\n", iti->nb_points);
        return -1;
    }

    // Allocation du buffer pour tous les points
    size_t points_size = iti->nb_points * sizeof(Point_iti);
    char* buffer = malloc(points_size);
    if (!buffer) {
        perror("malloc points buffer");
        return -1;
    }

    // Lecture de tous les points
    if (recvBuffer(buffer, points_size) <= 0) {
        free(buffer);
        return -1;
    }

    // Allocation dynamique pour iti->points
    iti->points = malloc(points_size);
    if (!iti->points) {
        perror("malloc iti->points");
        free(buffer);
        return -1;
    }

    memcpy(iti->points, buffer, points_size);
    free(buffer);

    return iti->nb_points;
}

// ==========================================================
// ===============   RECEPTION D’UN MESSAGE FIN   =========
// ==========================================================

int recvFin_ihm(int* voiture_id, char* buffer, size_t max_size) {
    MessageType type;
    int n = recvMessage_ihm(voiture_id, &type, buffer, max_size);
    if (n > 0)
        buffer[n < (int)max_size ? n : (int)max_size - 1] = '\0';
    return n;
}

// ==========================================================
// ===============   COMMUNICATION PRINCIPALE   ==========
// ==========================================================

void* communication_ihm(void* arg) {
    (void)arg;
    int voiture_id;
    MessageType type;
    char buffer[2048];
    Itineraire iti;

    printf("[Serveur] Communication avec l'IHM démarrée.\n");

    while (1) {
        int nbytes = recvMessage_ihm(&voiture_id, &type, buffer, sizeof(buffer));
        if (nbytes <= 0) {
            printf("[Serveur] Connexion interrompue.\n");
            break;
        }

        switch (type) {
            case MESSAGE_ITINERAIRE: {
                int nb = recvItineraire_ihm(&voiture_id, &iti);
                if (nb > 0) {
                    printf("[IHM] Itinéraire reçu pour voiture %d (%d points)\n", voiture_id, iti.nb_points);
                    for (int i = 0; i < iti.nb_points; i++) {
                        Point_iti* p = &iti.points[i];
                        printf("  P%d: x=%.2f y=%.2f z=%.2f theta=%.2f pont=%d dep=%d\n",
                               i, p->x, p->y, p->z, p->theta, p->pont, p->depacement);
                    }

                    set_voiture_itineraire(voiture_id, &iti);
                    free(iti.points);
                }
                break;
            }

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
// ===============   INITIALISATION SERVEUR   ============
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

    pthread_t tid;
    if (pthread_create(&tid, NULL, communication_ihm, NULL) != 0) {
        perror("Erreur pthread_create lancer réception ihm");
        return NULL;
    }

    close(se);
    return NULL;
}
