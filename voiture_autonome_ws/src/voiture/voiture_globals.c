// Ce fichier contient uniquement les variables globales propres au processus "voiture". 
// Aucune donnée n'est partagée avec le processus "controleur".
#define _POSIX_C_SOURCE 199309L
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


/* ==== Fonctions helper ==== */
static int lock_mutex(pthread_mutex_t* mtx, bool blocking) {
    return blocking ? pthread_mutex_lock(mtx) : pthread_mutex_trylock(mtx);
}

/* ==== Implémentation des set/get ==== */

// Itineraire
int set_itineraire(const Itineraire* t, int blocking) {
    if (check_initialized() != 0 || !t) return -1;
    if (lock_mutex(&g.itineraire.mutex, blocking) != 0) return -1;
    g.itineraire.data = *t;
    clock_gettime(CLOCK_MONOTONIC, &g.itineraire.last_update);
    pthread_mutex_unlock(&g.itineraire.mutex);
    return 0;
}

int get_itineraire(Itineraire* t, int blocking) {
    if (check_initialized() != 0 || !t) return -1;
    if (lock_mutex(&g.itineraire.mutex, blocking) != 0) return -1;
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
int set_consigne(const Consigne* t, int blocking) {
    if (check_initialized() != 0 || !t) return -1;
    if (lock_mutex(&g.consigne.mutex, blocking) != 0) return -1;
    g.consigne.data = *t;
    clock_gettime(CLOCK_MONOTONIC, &g.consigne.last_update);
    pthread_mutex_unlock(&g.consigne.mutex);
    return 0;
}

int get_consigne(Consigne* t, int blocking) {
    if (check_initialized() != 0 || !t) return -1;
    if (lock_mutex(&g.consigne.mutex, blocking) != 0) return -1;
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
int set_demande(const Demande* t, int blocking) {
    if (check_initialized() != 0 || !t) return -1;
    if (lock_mutex(&g.demande.mutex, blocking) != 0) return -1;
    g.demande.data = *t;
    clock_gettime(CLOCK_MONOTONIC, &g.demande.last_update);
    pthread_mutex_unlock(&g.demande.mutex);
    return 0;
}

int get_demande(Demande* t, int blocking) {
    if (check_initialized() != 0 || !t) return -1;
    if (lock_mutex(&g.demande.mutex, blocking) != 0) return -1;
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
int set_etat(const EtatVoiture* t, int blocking) {
    if (check_initialized() != 0 || !t) return -1;
    if (lock_mutex(&g.etat_voiture.mutex, blocking) != 0) return -1;
    g.etat_voiture.data = *t;
    clock_gettime(CLOCK_MONOTONIC, &g.etat_voiture.last_update);
    pthread_mutex_unlock(&g.etat_voiture.mutex);
    return 0;
}

int get_etat(EtatVoiture* t, int blocking) {
    if (check_initialized() != 0 || !t) return -1;
    if (lock_mutex(&g.etat_voiture.mutex, blocking) != 0) return -1;
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
int set_position(const PositionVoiture* t, int blocking) {
    if (check_initialized() != 0 || !t) return -1;
    if (lock_mutex(&g.position_voiture.mutex, blocking) != 0) return -1;
    g.position_voiture.data = *t;
    clock_gettime(CLOCK_MONOTONIC, &g.position_voiture.last_update);
    pthread_mutex_unlock(&g.position_voiture.mutex);
    return 0;
}

int get_position(PositionVoiture* t, int blocking) {
    if (check_initialized() != 0 || !t) return -1;
    if (lock_mutex(&g.position_voiture.mutex, blocking) != 0) return -1;
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
int set_trajectoire(const Trajectoire* t, int blocking) {
    if (check_initialized() != 0 || !t) return -1;
    if (lock_mutex(&g.trajectoire.mutex, blocking) != 0) return -1;
    g.trajectoire.data = *t;
    clock_gettime(CLOCK_MONOTONIC, &g.trajectoire.last_update);
    pthread_mutex_unlock(&g.trajectoire.mutex);
    return 0;
}

int get_trajectoire(Trajectoire* t, int blocking) {
    if (check_initialized() != 0 || !t) return -1;
    if (lock_mutex(&g.trajectoire.mutex, blocking) != 0) return -1;
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
int set_donnees_detection(const DonneesDetection* t, int blocking) {
    if (check_initialized() != 0 || !t) return -1;
    if (lock_mutex(&g.donnees_detection.mutex, blocking) != 0) return -1;
    g.donnees_detection.data = *t;
    clock_gettime(CLOCK_MONOTONIC, &g.donnees_detection.last_update);
    pthread_mutex_unlock(&g.donnees_detection.mutex);
    return 0;
}

int get_donnees_detection(DonneesDetection* t, int blocking) {
    if (check_initialized() != 0 || !t) return -1;
    if (lock_mutex(&g.donnees_detection.mutex, blocking) != 0) return -1;
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
int set_sensor_data(const SensorData* t, int blocking) {
    if (check_initialized() != 0 || !t) return -1;
    if (lock_mutex(&g.sensor_data.mutex, blocking) != 0) return -1;
    g.sensor_data.data = *t;
    clock_gettime(CLOCK_MONOTONIC, &g.sensor_data.last_update);
    pthread_mutex_unlock(&g.sensor_data.mutex);
    return 0;
}

int get_sensor_data(SensorData* t, int blocking) {
    if (check_initialized() != 0 || !t) return -1;
    if (lock_mutex(&g.sensor_data.mutex, blocking) != 0) return -1;
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