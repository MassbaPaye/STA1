// localisation_fusion.h
#ifndef LOCALISATION_FUSION_H
#define LOCALISATION_FUSION_H

#include <stdbool.h>
#include <time.h>
#include "voiture_globals.h"
#include "marvelmind_manager.h"
#include "communication_serie.h"

typedef struct {
    int x;      // mm
    int y;      // mm
    int z;      // mm
    float theta; // degrés
    float vx;   // mm/s
    float vy;   // mm/s
    float vz;   // mm/s
} PositionOdom;


// --- Fonctions principales ---

// Calcule une position odométrique à partir des dernières données capteurs
PositionOdom calculer_odometrie(void);

// Met à jour la position Marvelmind estimée (fusion + projection)
MarvelmindPosition mettre_a_jour_marvelmind_estimee(const PositionOdom* odom);

void estimer_position_apres_dt(float* x, float* y, float* z, float vx, float vy, float vz, double dt);


#endif
