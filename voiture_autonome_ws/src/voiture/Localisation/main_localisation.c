#include "utils.h"
#include <stdio.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include <stdbool.h>
#include "main_localisation.h"
#include "logger.h"
#include "marvelmind_manager.h"
#include "localisation_fusion.h"
#include "voiture_globals.h"
#include "config.h"

#define TAG "loc-main"

#define POSITION_LOG_FILE "position_log.csv"

#define LOCALISATION_FREQ_HZ 20
#define LOCALISATION_DT (1.0 / LOCALISATION_FREQ_HZ)

bool running = false;
PositionVoiture pos_globale = {0};
PositionOdom odom_pos_estimee = {0};
MarvelmindPosition mm_pos_estimee = {0};

void update_localisation_ponderation() 
{
    odom_pos_estimee = calculer_odometrie();
    mm_pos_estimee = mettre_a_jour_marvelmind_estimee(&odom_pos_estimee);
    INFO(TAG, "valid :%d", mm_pos_estimee.valid);
    INFO(TAG, "USEMARVEL :%d", USE_MARVELMIND);

    // --- Fusion pondérée finale ---
    if (mm_pos_estimee.valid && USE_MARVELMIND) {
        INFO(TAG, "x:%.1f, y%.1f", mm_pos_estimee.x, mm_pos_estimee.y);
        // Fusion pondérée entre odométrie et Marvelmind estimé
        pos_globale.x = FUSION_POSITION_GLOBALE_WEIGHT * odom_pos_estimee.x + 
                        (1.0 - FUSION_POSITION_GLOBALE_WEIGHT) * mm_pos_estimee.x;
        pos_globale.y = FUSION_POSITION_GLOBALE_WEIGHT * odom_pos_estimee.y + 
                        (1.0 - FUSION_POSITION_GLOBALE_WEIGHT) * mm_pos_estimee.y;
        pos_globale.z = FUSION_POSITION_GLOBALE_WEIGHT * odom_pos_estimee.z + 
                        (1.0 - FUSION_POSITION_GLOBALE_WEIGHT) * mm_pos_estimee.z;
    } else {
        // Odométrie seule si pas de Marvelmind valide
        pos_globale.x = odom_pos_estimee.x;
        pos_globale.y = odom_pos_estimee.y;
        pos_globale.z = odom_pos_estimee.z;
    }

    pos_globale.vx = odom_pos_estimee.vx;
    pos_globale.vy = odom_pos_estimee.vy;
    pos_globale.vz = odom_pos_estimee.vz;
    pos_globale.theta = odom_pos_estimee.theta;

    // --- Mise à jour de la position globale partagée ---
    PositionVoiture pos;
    get_position(&pos);
    
    FILE* f = fopen(POSITION_LOG_FILE, "a"); // mode "append"
    if (f) {
        struct timespec t_now;
        clock_gettime(CLOCK_MONOTONIC, &t_now);
        double timestamp_s = t_now.tv_sec + t_now.tv_nsec * 1e-9;
        fprintf(f, "%.6f, %.3f, %.3f, %.3f, %.2f\n",
                timestamp_s,
                pos_globale.x,
                pos_globale.y,
                pos_globale.z,
                pos_globale.theta);
        fclose(f);
    } else {
        ERR(TAG, "Impossible d'ouvrir le fichier de log %s", POSITION_LOG_FILE);
    }
    set_position(&pos_globale);

}


void* lancer_localisation_thread(void* arg) {
    (void)arg;
    running = true;

    FILE* f = fopen(POSITION_LOG_FILE, "w"); // mode "append"
    if (f) {
        fprintf(f, "t,x,y,z,theta\n");
        fclose(f);
    } else {
        ERR(TAG, "Impossible d'ouvrir le fichier de log %s", POSITION_LOG_FILE);
    }

    if (USE_MARVELMIND) {
        if (lancer_marvelmind() != 0) {
            ERR(TAG, "Impossible de démarrer le module Marvelmind.");
            return NULL;
        }
    } else {
        WARN(TAG, "Marvelmind désactivé (USE_MARVELMIND=0).");
    }

    while(running) {       
        #ifdef DEBUG_LOC
        struct timespec t_before, t_after;
        clock_gettime(CLOCK_MONOTONIC, &t_before);
        update_localisation_ponderation();
        clock_gettime(CLOCK_MONOTONIC, &t_after);
        double dt_exec = timespec_diff_s(t_before, t_after);
        DBG(TAG, "Cycle localisation exécuté en %.7fs => x=%.1f y=%.1f (odom_pos_estimee θ=%.1f°, marvelmind valid=%d)",
            dt_exec, pos_globale.x, pos_globale.y, odom_pos_estimee.theta, mm_pos_estimee.valid);
        #else
        update_localisation_ponderation();
        #endif
        my_sleep(LOCALISATION_DT);
    }

    INFO(TAG, "Thread de localisation terminé.");
    return NULL;
}

void stop_localisation() {
    running = false;
}