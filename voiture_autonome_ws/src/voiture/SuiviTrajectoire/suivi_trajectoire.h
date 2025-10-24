#ifndef SUIVI_TRAJECTOIRE_H
#define SUIVI_TRAJECTOIRE_H

#include <stdbool.h>
#include "messages.h"



// ====================== CONSTANTES ======================
#define K0 0.05f
#define L1 40.0f

// ====================== INTERFACE PUBLIQUE ======================
void* lancer_suivi_trajectoire(void* arg);

// Fonctions externes à implémenter dans ton environnement :
int get_trajectoire(Trajectoire* t);
int get_position(PositionVoiture* p);
void set_motor_speed(float v_left, float v_right);

#endif

/*

#ifndef CONTROL_H
#define CONTROL_H

#include <stdbool.h>

// ====================== STRUCTURES ======================
#define MAX_POINTS_TRAJECTOIRE 100

typedef struct {
    float x;     // mm
    float y;     // mm
    float z;     // mm
    float theta; // degrés
} Point;

typedef struct {
    int nb_points;
    Point points[MAX_POINTS_TRAJECTOIRE];
    float vitesse;      // mm/s
    float vitesse_max;  // mm/s
    bool arreter_fin;
} Trajectoire;

typedef struct {
    float x;     // mm
    float y;     // mm
    float z;     // mm
    float theta; // degrés
    float vx;
    float vy;
    float vz;
} PositionVoiture;

// ====================== CONSTANTES ======================
#define V_NOMINALE 100.0f   // mm/s
#define K0 0.05f
#define L1 40.0f            // mm
#define RAYON_ROUE 30.0f    // mm (à adapter)
#define DEMI_ENTRE_ROUES 70.0f  // mm (à adapter)

// ====================== PROTOTYPES ======================
void compute_local_polynomial(Point *p1, Point *p2, Point *p3, float coeffs[4]);
void compute_projection(float x, float y, float coeffs[4], float *xs, float *ys);
void compute_errors(float x, float y, float theta_deg, float coeffs[4], float *d, float *theta_e);
float compute_omega(float v_ref, float d, float theta_e);
void compute_wheel_speeds(float v, float omega, float *w_left, float *w_right);

#endif
*/
