// Ce fichier contient uniquement les variables globales propres au processus "voiture". 
// Aucune donnée n'est partagée avec le processus "controleur".
#include "utils.h"
#include "voiture_globals.h"
#include <time.h>
#include "logger.h"


#define TAG "voiture_globals"

const struct timespec TIMESPEC_UNDEFINED = {0, 0};
static int initialized = 0;

static inline int check_initialized(void) {
    if (initialized) {
        return 0;
    }
    DBG(TAG, "Variable globale non initialisée");
    return -1;
}

/* ==== Instance globale ==== */
GlobalsVoiture g = {
    .itineraire.mutex = PTHREAD_MUTEX_INITIALIZER,
    .consigne.mutex = PTHREAD_MUTEX_INITIALIZER,
    .demande.mutex = PTHREAD_MUTEX_INITIALIZER,
    .etat_voiture.mutex = PTHREAD_MUTEX_INITIALIZER,
    .position_voiture.mutex = PTHREAD_MUTEX_INITIALIZER,
    .trajectoire.mutex = PTHREAD_MUTEX_INITIALIZER,
    .donnees_detection.mutex = PTHREAD_MUTEX_INITIALIZER
};


void init_voiture_globals(void) {
    if (initialized) return;  // déjà initialisé

    pthread_mutex_init(&g.itineraire.mutex, NULL);
    pthread_mutex_init(&g.consigne.mutex, NULL);
    pthread_mutex_init(&g.demande.mutex, NULL);
    pthread_mutex_init(&g.etat_voiture.mutex, NULL);
    pthread_mutex_init(&g.position_voiture.mutex, NULL);
    pthread_mutex_init(&g.trajectoire.mutex, NULL);
    pthread_mutex_init(&g.donnees_detection.mutex, NULL);

    g.itineraire.last_update = TIMESPEC_UNDEFINED;
    g.consigne.last_update = TIMESPEC_UNDEFINED;
    g.demande.last_update = TIMESPEC_UNDEFINED;
    g.etat_voiture.last_update = TIMESPEC_UNDEFINED;
    g.position_voiture.last_update = TIMESPEC_UNDEFINED;
    g.trajectoire.last_update = TIMESPEC_UNDEFINED;
    g.donnees_detection.last_update = TIMESPEC_UNDEFINED;

    initialized = 1;
}

/* ==== Implémentation des set/get ==== */

// Itineraire
int set_itineraire(const Itineraire* t) {
    if (check_initialized() != 0 || !t) return -1;
    pthread_mutex_lock(&g.itineraire.mutex);
    g.itineraire.data = *t;
    clock_gettime(CLOCK_MONOTONIC, &g.itineraire.last_update);
    pthread_mutex_unlock(&g.itineraire.mutex);
    return 0;
}

int get_itineraire(Itineraire* t) {
    if (check_initialized() != 0 || !t) return -1;
    pthread_mutex_lock(&g.itineraire.mutex);
    *t = g.itineraire.data;
    pthread_mutex_unlock(&g.itineraire.mutex);
    return 0;
}

struct timespec get_itineraire_last_update(void) {
    struct timespec ts = TIMESPEC_UNDEFINED;
    pthread_mutex_lock(&g.itineraire.mutex);
    ts = g.itineraire.last_update;
    pthread_mutex_unlock(&g.itineraire.mutex);
    return ts;
}

// Consigne
int set_consigne(const Consigne* t) {
    if (check_initialized() != 0 || !t) return -1;
    pthread_mutex_lock(&g.consigne.mutex);
    g.consigne.data = *t;
    clock_gettime(CLOCK_MONOTONIC, &g.consigne.last_update);
    pthread_mutex_unlock(&g.consigne.mutex);
    return 0;
}

int get_consigne(Consigne* t) {
    if (check_initialized() != 0 || !t) return -1;
    pthread_mutex_lock(&g.consigne.mutex);
    *t = g.consigne.data;
    pthread_mutex_unlock(&g.consigne.mutex);
    return 0;
}

struct timespec get_consigne_last_update(void) {
    struct timespec ts = TIMESPEC_UNDEFINED;
    pthread_mutex_lock(&g.consigne.mutex);
    ts = g.consigne.last_update;
    pthread_mutex_unlock(&g.consigne.mutex);
    return ts;
}

// Demande
int set_demande(const Demande* t) {
    if (check_initialized() != 0 || !t) return -1;
    pthread_mutex_lock(&g.demande.mutex);
    g.demande.data = *t;
    clock_gettime(CLOCK_MONOTONIC, &g.demande.last_update);
    pthread_mutex_unlock(&g.demande.mutex);
    return 0;
}

int get_demande(Demande* t) {
    if (check_initialized() != 0 || !t) return -1;
    pthread_mutex_lock(&g.demande.mutex);
    *t = g.demande.data;
    pthread_mutex_unlock(&g.demande.mutex);
    return 0;
}

struct timespec get_demande_last_update(void) {
    struct timespec ts = TIMESPEC_UNDEFINED;
    pthread_mutex_lock(&g.demande.mutex);
    ts = g.demande.last_update;
    pthread_mutex_unlock(&g.demande.mutex);
    return ts;
}

// EtatVoiture
int set_etat(const EtatVoiture* t) {
    if (check_initialized() != 0 || !t) return -1;
    pthread_mutex_lock(&g.etat_voiture.mutex);
    g.etat_voiture.data = *t;
    clock_gettime(CLOCK_MONOTONIC, &g.etat_voiture.last_update);
    pthread_mutex_unlock(&g.etat_voiture.mutex);
    return 0;
}

int get_etat(EtatVoiture* t) {
    if (check_initialized() != 0 || !t) return -1;
    pthread_mutex_lock(&g.etat_voiture.mutex);
    *t = g.etat_voiture.data;
    pthread_mutex_unlock(&g.etat_voiture.mutex);
    return 0;
}

struct timespec get_etat_last_update(void) {
    struct timespec ts = TIMESPEC_UNDEFINED;
    pthread_mutex_lock(&g.etat_voiture.mutex);
    ts = g.etat_voiture.last_update;
    pthread_mutex_unlock(&g.etat_voiture.mutex);
    return ts;
}

// PositionVoiture
int set_position(const PositionVoiture* t) {
    if (check_initialized() != 0 || !t) return -1;
    pthread_mutex_lock(&g.position_voiture.mutex);
    g.position_voiture.data = *t;
    clock_gettime(CLOCK_MONOTONIC, &g.position_voiture.last_update);
    pthread_mutex_unlock(&g.position_voiture.mutex);
    return 0;
}

int get_position(PositionVoiture* t) {
    if (check_initialized() != 0 || !t) return -1;
    pthread_mutex_lock(&g.position_voiture.mutex);
    *t = g.position_voiture.data;
    pthread_mutex_unlock(&g.position_voiture.mutex);
    return 0;
}

struct timespec get_position_last_update(void) {
    struct timespec ts = TIMESPEC_UNDEFINED;
    pthread_mutex_lock(&g.position_voiture.mutex);
    ts = g.position_voiture.last_update;
    pthread_mutex_unlock(&g.position_voiture.mutex);
    return ts;
}

// Trajectoire
int set_trajectoire(const Trajectoire* t) {
    if (check_initialized() != 0 || !t) return -1;
    pthread_mutex_lock(&g.trajectoire.mutex);
    g.trajectoire.data = *t;
    clock_gettime(CLOCK_MONOTONIC, &g.trajectoire.last_update);
    pthread_mutex_unlock(&g.trajectoire.mutex);
    return 0;
}

int get_trajectoire(Trajectoire* t) {
    if (check_initialized() != 0 || !t) return -1;
    pthread_mutex_lock(&g.trajectoire.mutex);
    *t = g.trajectoire.data;
    pthread_mutex_unlock(&g.trajectoire.mutex);
    return 0;
}

struct timespec get_trajectoire_last_update(void) {
    struct timespec ts = TIMESPEC_UNDEFINED;
    pthread_mutex_lock(&g.trajectoire.mutex);
    ts = g.trajectoire.last_update;
    pthread_mutex_unlock(&g.trajectoire.mutex);
    return ts;
}

// DonneesDetection
int set_donnees_detection(const DonneesDetection* t) {
    if (check_initialized() != 0 || !t) return -1;
    pthread_mutex_lock(&g.donnees_detection.mutex);
    g.donnees_detection.data = *t;
    clock_gettime(CLOCK_MONOTONIC, &g.donnees_detection.last_update);
    pthread_mutex_unlock(&g.donnees_detection.mutex);
    return 0;
}

int get_donnees_detection(DonneesDetection* t) {
    if (check_initialized() != 0 || !t) return -1;
    pthread_mutex_lock(&g.donnees_detection.mutex);
    *t = g.donnees_detection.data;
    pthread_mutex_unlock(&g.donnees_detection.mutex);
    return 0;
}

struct timespec get_donnees_detection_last_update(void) {
    struct timespec ts = TIMESPEC_UNDEFINED;
    pthread_mutex_lock(&g.donnees_detection.mutex);
    ts = g.donnees_detection.last_update;
    pthread_mutex_unlock(&g.donnees_detection.mutex);
    return ts;
}



// SensorData
int set_sensor_data(const SensorData* t) {
    if (check_initialized() != 0 || !t) return -1;
    pthread_mutex_lock(&g.sensor_data.mutex);
    g.sensor_data.data = *t;
    clock_gettime(CLOCK_MONOTONIC, &g.sensor_data.last_update);
    pthread_mutex_unlock(&g.sensor_data.mutex);
    return 0;
}

int get_sensor_data(SensorData* t) {
    if (check_initialized() != 0 || !t) return -1;
    pthread_mutex_lock(&g.sensor_data.mutex);
    *t = g.sensor_data.data;
    pthread_mutex_unlock(&g.sensor_data.mutex);
    return 0;
}

struct timespec get_sensor_data_last_update(void) {
    struct timespec ts = TIMESPEC_UNDEFINED;
    pthread_mutex_lock(&g.sensor_data.mutex);
    ts = g.sensor_data.last_update;
    pthread_mutex_unlock(&g.sensor_data.mutex);
    return ts;
}