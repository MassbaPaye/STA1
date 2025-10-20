#define _POSIX_C_SOURCE 199309L
#include <unistd.h>
#include "marvelmind_manager.h"
#include "utils.h"
#include "logger.h"
#include <pthread.h> // Attention à bien inclure avant marvelmind.h
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "marvelmind.h"

#define TAG "loc-marvelmind"
#define RECONNECT_DELAY_SEC 5
#define GET_POSITION_FREQ 16 // Hz

// ===========================
// Variables internes
// ===========================

static pthread_t marvelmind_thread;
static bool running = false;

static pthread_mutex_t pos_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t pos_cond = PTHREAD_COND_INITIALIZER;

static MarvelmindPosition current_position = {0};

// ===========================
// Thread principal
// ===========================

static void *marvelmind_task(void *arg) {
    (void)arg;  // inutilisé
    bool was_connected = true; // Pour afficher un warn au début s'il y a un echec de connexion

    while (running) {
        struct MarvelmindHedge *hedge = createMarvelmindHedge();
        if (!hedge) {
            ERR(TAG, "Impossible de créer le hedge Marvelmind");
            sleep(RECONNECT_DELAY_SEC);
            continue;
        }

        hedge->ttyFileName = marvelmind_port;
        hedge->verbose = false;
        hedge->terminationRequired = false;

        startMarvelmindHedge(hedge);
        
        struct PositionValue pos = {0};
        
        while (running && !hedge->terminationRequired) {
            if (getPositionFromMarvelmindHedge(hedge, &pos)) {
                pthread_mutex_lock(&pos_mutex);
                current_position.x = (float) pos.x;
                current_position.y = (float) pos.y;
                current_position.z = (float) pos.z;
                clock_gettime(CLOCK_REALTIME, &current_position.t);
                current_position.valid = true;
                pthread_cond_broadcast(&pos_cond);
                pthread_mutex_unlock(&pos_mutex);
                // Reconnexion réussie
                if (!was_connected) {
                    INFO(TAG, "Marvelmind connecté sur %s", marvelmind_port);
                    was_connected = true;
                }
            } else {
                 // Perte de connexion détectée
                if (was_connected) {
                    WARN(TAG, "Marvelmind perdu (%s), tentative de reconnexion toute les %d s", marvelmind_port, RECONNECT_DELAY_SEC);
                    was_connected = false;
                }
                struct timespec ts;
                ts.tv_sec = 0;
                ts.tv_nsec = (int) 1/GET_POSITION_FREQ * 1e9; // 100 ms
                nanosleep(&ts, NULL);
            }
        }
        if (!running)
            INFO(TAG, "Déconnexion du Marvelmind (%s).");
        stopMarvelmindHedge(hedge);
        destroyMarvelmindHedge(hedge);
        hedge = NULL;

        if (running) {
            sleep(RECONNECT_DELAY_SEC);
        }
    }

    DBG(TAG, "Thread Marvelmind terminé.");
    return NULL;
}

// ===========================
// Fonctions publiques
// ===========================

int start_marvelmind() {
    if (running) {
        ERR(TAG, "Thread Marvelmind déjà actif");
        return -1;
    }

    if (marvelmind_port == NULL) {
        ERR(TAG, "Port Marvelmind non défini (marvelmind_port est NULL)");
        return -1;
    }

    running = true;
    current_position.valid = false;

    int res = pthread_create(&marvelmind_thread, NULL, marvelmind_task, NULL);
    if (res != 0) {
        ERR(TAG, "Erreur création thread Marvelmind : %s", strerror(res));
        running = false;
        return -1;
    }

    INFO(TAG, "Thread Marvelmind lancé avec succès.");
    return 0;
}

void stop_marvelmind() {
    if (!running)
        return;

    running = false;
    pthread_cond_broadcast(&pos_cond); // débloque un éventuel wait_for_position()

    pthread_join(marvelmind_thread, NULL);
    INFO(TAG, "Thread Marvelmind arrêté proprement.");
}

int wait_for_position(int timeout_sec) {
    struct timespec ts;
    int ret = 0;

    pthread_mutex_lock(&pos_mutex);

    if (timeout_sec > 0) {
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec += timeout_sec;
        ret = pthread_cond_timedwait(&pos_cond, &pos_mutex, &ts);
    } else if (timeout_sec == 0) {
        ret = ETIMEDOUT;
    } else {
        pthread_cond_wait(&pos_cond, &pos_mutex);
    }

    pthread_mutex_unlock(&pos_mutex);

    return (ret == 0) ? 0 : -1;
}

MarvelmindPosition get_marvelmind_position() {
    pthread_mutex_lock(&pos_mutex);
    MarvelmindPosition pos_copy = current_position;
    pthread_mutex_unlock(&pos_mutex);
    return pos_copy;
}

void _set_marvelmind_position(MarvelmindPosition pos) {
    pthread_mutex_lock(&pos_mutex);
    current_position = pos;
    pthread_mutex_unlock(&pos_mutex);
}
