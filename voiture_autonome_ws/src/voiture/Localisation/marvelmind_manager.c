
#include "utils.h"
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
#include "marvelmind_manager.h"

#define TAG "loc-marvelmind"
#define RECONNECT_DELAY_SEC 5
#define GET_POSITION_FREQ 16 // Hz

// ===========================
// Variables internes
// ===========================

static bool running = false;

static pthread_mutex_t pos_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t pos_cond = PTHREAD_COND_INITIALIZER;

static MarvelmindPosition current_position = {0};
struct MarvelmindHedge *hedge;

// ===========================
// Thread principal
// ===========================

static void positionCallback(struct PositionValue position) {
    //INFO(TAG, "Position marvelmind recue");
    pthread_mutex_lock(&pos_mutex);
    current_position.x = (float) position.x;
    current_position.y = (float) position.y;
    current_position.z = (float) position.z;
    clock_gettime(CLOCK_REALTIME, &current_position.t);
    current_position.valid = true;
    current_position.is_new = true;
    pthread_mutex_unlock(&pos_mutex);
    pthread_cond_broadcast(&pos_cond);
}




// ===========================
// Fonctions publiques
// ===========================

int lancer_marvelmind() {
    if (running) {
        ERR(TAG, "Marvelmind déjà lancé");
        return -1;
    }
    hedge = createMarvelmindHedge();

    if (hedge == NULL) {
        ERR(TAG, "Impossible de créer le hedge Marvelmind");
        running = false;
        return -1;
    }
    if (marvelmind_port == NULL) {
        ERR(TAG, "port_marvelmind est NULL ");
        running = false;
        hedge = NULL;
        return -1;
    }
    running = true;
    current_position.valid = false;

    hedge->ttyFileName = marvelmind_port;
    // Surement à modifier 
    #ifdef DEBUG_LOC
    hedge->verbose = true;
    #else
    hedge->verbose = false;
    #endif
    hedge->terminationRequired = false;
    hedge->receiveDataCallback = positionCallback; // a definir

    startMarvelmindHedge(hedge);
    INFO(TAG, "Marvelmind thread lancé, thread_id=%lu", hedge->thread_);
    
    INFO(TAG, "Thread Marvelmind lancé avec succès sur %s.", marvelmind_port);
    return 0;
}


void stop_marvelmind() {
    if (!running)
        return;

    running = false;
    pthread_cond_broadcast(&pos_cond); // débloque un éventuel wait_for_position()
    if (hedge) {
        stopMarvelmindHedge(hedge);
        destroyMarvelmindHedge(hedge);
        hedge = NULL;
    }

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
        ret = -1;
    } else {
        pthread_cond_wait(&pos_cond, &pos_mutex);
    }

    pthread_mutex_unlock(&pos_mutex);

    return (ret == 0) ? 0 : -1;
}


MarvelmindPosition get_marvelmind_position(bool change_to_read) {
    //struct PositionValue pos;
    //int r = getPositionFromMarvelmindHedge(hedge, &pos);
    //INFO(TAG, "r=%d, x:%d", r, pos.x);

    pthread_mutex_lock(&pos_mutex);
    current_position.is_new = false;
    MarvelmindPosition pos_copy = current_position;
    pthread_mutex_unlock(&pos_mutex);
    return pos_copy;
}

void _set_marvelmind_position(MarvelmindPosition pos) {
    pthread_mutex_lock(&pos_mutex);
    current_position = pos;
    pthread_mutex_unlock(&pos_mutex);
}

