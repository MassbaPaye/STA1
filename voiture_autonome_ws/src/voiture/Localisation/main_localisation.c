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
#include "utils.h"

#define TAG "localisation"


#define LOCALISATION_FREQ_HZ 10
#define LOCALISATION_DT (1.0 / LOCALISATION_FREQ_HZ)

bool running = false;
PositionVoiture pos_globale = {0};
PositionOdom odom_pos_estimee = {0};
MarvelmindPosition mm_pos_estimee = {0};

void update_localisation_ponderation() 
{
    odom_pos_estimee = calculer_odometrie();
    mm_pos_estimee = mettre_a_jour_marvelmind_estimee(&odom_pos_estimee);

    // --- Fusion pondérée finale ---
    if (mm_pos_estimee.valid && USE_MARVELMIND) {
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
    set_position(&pos_globale);

}


void* lancer_localisation_thread() {
    running = true;

    INFO(TAG, "Thread de localisation démarré.");


    if (USE_MARVELMIND) {
        if (start_marvelmind(marvelmind_port) != 0) {
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