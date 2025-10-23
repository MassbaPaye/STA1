// Ce fichier contient uniquement les variables globales propres au processus "controleur". 
// Aucune donnée n'est partagée avec le processus "voiture".

#include "utils.h"
#include "controleur_globals.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "logger.h"
#include "communication_tcp_controleur.h"
#include "communication_tcp_ihm.h"
#include "messages.h"

#define TAG "contorleur_globals"

/* ==== Déclaration des variables globales ==== */
const struct timespec TIMESPEC_UNDEFINED = {0, 0};

Voiture voitures[MAX_VOITURES];
Queue demandes_queue;
EtatStructure structure[MAX_STRUCTURE];

/* ==== Initialisation des mutex et queue ==== */
void init_controleur_globals(void) {
    for (int i = 0; i < MAX_VOITURES; i++) {
        pthread_mutex_init(&voitures[i].mutex, NULL);

        voitures[i].etat_last_update = TIMESPEC_UNDEFINED;
        voitures[i].position_last_update = TIMESPEC_UNDEFINED;
        voitures[i].itineraire_last_update = TIMESPEC_UNDEFINED;
    }
    // Initialise la queue globale (ex: capacité 100 demandes)
    if (queue_init(&demandes_queue, 100, sizeof(Demande)) != 0) {
        ERR(TAG, "Erreur init demandes_queue\n");
        exit(EXIT_FAILURE);
    }
}


/* ==== Etat ==== */
int set_voiture_etat(int id, const EtatVoiture* etat) {
    if (!etat || id < 0 || id >= MAX_VOITURES) return -1;
    pthread_mutex_lock(&voitures[id].mutex);
    voitures[id].etat = *etat;
    clock_gettime(CLOCK_MONOTONIC, &voitures[id].etat_last_update);
    pthread_mutex_unlock(&voitures[id].mutex);
    return 0;
}

int get_voiture_etat(int id, EtatVoiture* etat) {
    if (!etat || id < 0 || id >= MAX_VOITURES) return -1;
    pthread_mutex_lock(&voitures[id].mutex);
    *etat = voitures[id].etat;
    pthread_mutex_unlock(&voitures[id].mutex);
    return 0;
}

struct timespec get_voiture_etat_last_update(int id) {
    struct timespec ts = TIMESPEC_UNDEFINED;
    if (id < 0 || id >= MAX_VOITURES) return ts;
    pthread_mutex_lock(&voitures[id].mutex);
    ts = voitures[id].etat_last_update;
    pthread_mutex_unlock(&voitures[id].mutex);
    return ts;
}


/* ==== Position ==== */
int set_voiture_position(int id, const PositionVoiture* pos) {
    if (!pos || id < 0 || id >= MAX_VOITURES) return -1;
    pthread_mutex_lock(&voitures[id].mutex);
    voitures[id].position = *pos;
    clock_gettime(CLOCK_MONOTONIC, &voitures[id].position_last_update);
    sendMessage_ihm(id, MESSAGE_POSITION, pos, sizeof(PositionVoiture));
    pthread_mutex_unlock(&voitures[id].mutex);
    return 0;
}

int get_voiture_position(int id, PositionVoiture* pos) {
    if (!pos || id < 0 || id >= MAX_VOITURES) return -1;
    pthread_mutex_lock(&voitures[id].mutex);
    *pos = voitures[id].position;
    pthread_mutex_unlock(&voitures[id].mutex);
    return 0;
}

struct timespec get_voiture_position_last_update(int id) {
    struct timespec ts = TIMESPEC_UNDEFINED;
    if (id < 0 || id >= MAX_VOITURES) return ts;
    pthread_mutex_lock(&voitures[id].mutex);
    ts = voitures[id].position_last_update;
    pthread_mutex_unlock(&voitures[id].mutex);
    return ts;
}

/* ==== Itineraire ==== */
int set_voiture_itineraire(int id, const Itineraire* itin) {
    if (!itin || id < 0 || id >= MAX_VOITURES) return -1;
    pthread_mutex_lock(&voitures[id].mutex);
    voitures[id].itineraire = *itin;
    clock_gettime(CLOCK_MONOTONIC, &voitures[id].itineraire_last_update);
    sendMessage(id, MESSAGE_ITINERAIRE, &itin);
    pthread_mutex_unlock(&voitures[id].mutex);
    return 0;
}

int get_voiture_itineraire(int id, Itineraire* itin) {
    if (!itin || id < 0 || id >= MAX_VOITURES) return -1;
    pthread_mutex_lock(&voitures[id].mutex);
    *itin = voitures[id].itineraire;
    pthread_mutex_unlock(&voitures[id].mutex);
    return 0;
}

struct timespec get_voiture_itineraire_last_update(int id) {
    struct timespec ts = TIMESPEC_UNDEFINED;
    if (id < 0 || id >= MAX_VOITURES) return ts;
    pthread_mutex_lock(&voitures[id].mutex);
    ts = voitures[id].itineraire_last_update;
    pthread_mutex_unlock(&voitures[id].mutex);
    return ts;
}

/* ==== Queue globale demandes ==== */
int enqueue_demande(const Demande* d) {
    if (!d) return -1;
    return queue_enqueue(&demandes_queue, d);
}

int dequeue_demande(Demande* d) {
    if (!d) return -1;
    return queue_dequeue(&demandes_queue, d);
}

int size_demande_queue(void) {
    return queue_size(&demandes_queue);
}
