#include "suivi_trajectoire.h"
#include <math.h>
#include <stdio.h>
#include <unistd.h> // pour usleep()
#include "utils.h"
#include "config.h"
#include "logger.h"
#include "messages.h"


#define DEG2RAD(x) ((x) * PI / 180.0f)
#define RAD2DEG(x) ((x) * 180.0f / PI)

float theta_e;
float d;


void compute_frenet_coordinates_using_closest_point_only(PositionVoiture voiture, Point p) {
    d = - (voiture.x - p.x) * cos(p.theta) + (voiture.y - p.y) * sin(p.theta);
    theta_e = voiture.theta - p.theta; 
}
























// Calcul du projeté orthogonal par itération locale
static void compute_projection(float x, float y, const float coeffs[4], float *xs, float *ys) {
    float x_s = x;
    for (int i = 0; i < 10; ++i) {
        float y_s = coeffs[0] + coeffs[1]*x_s + coeffs[2]*x_s*x_s + coeffs[3]*x_s*x_s*x_s;
        float dy_dx = coeffs[1] + 2*coeffs[2]*x_s + 3*coeffs[3]*x_s*x_s;
        float f = (x - x_s) + (y_s - y)*dy_dx;
        float df = -1 + dy_dx*dy_dx + (y_s - y)*(2*coeffs[2] + 6*coeffs[3]*x_s);
        if (fabs(df) < 1e-6f) break;
        x_s -= f / df;
    }
    *xs = x_s;
    *ys = coeffs[0] + coeffs[1]*x_s + coeffs[2]*x_s*x_s + coeffs[3]*x_s*x_s*x_s;
}

// Calcul des erreurs latérale et angulaire
static void compute_errors(float x, float y, float theta_deg,
                           const float coeffs[4], float *d, float *theta_e) {
    float xs, ys;
    compute_projection(x, y, coeffs, &xs, &ys);
    float dy_dx = coeffs[1] + 2*coeffs[2]*xs + 3*coeffs[3]*xs*xs;
    float theta_s = atan(dy_dx);
    float nx = -sin(theta_s), ny = cos(theta_s);
    *d = (x - xs)*nx + (y - ys)*ny;
    float theta_r = DEG2RAD(theta_deg);
    *theta_e = theta_r - theta_s;
    while (*theta_e > PI) *theta_e -= 2*PI;
    while (*theta_e < -PI) *theta_e += 2*PI;
}

// Loi de commande
static float compute_omega(float v_ref, float d, float theta_e) {
    return -(v_ref / L1) * tanf(theta_e) - K0 * v_ref * d;
}

// Conversion vitesse/rotation → vitesses roues
static void compute_wheel_speeds(float v, float omega, float *v_left, float *v_right) {
    *v_left  = v - ECARTEMENT_ROUE * omega;
    *v_right = v + ECARTEMENT_ROUE * omega;
}

// ========== Thread de suivi de trajectoire ==========

void* lancer_suivi_trajectoire(void* arg) {
    (void)arg; // inutilisé
    Trajectoire traj;
    PositionVoiture pos;

    if (get_trajectoire(&traj) != 0) {
        printf("[suivi] Erreur de lecture de la trajectoire\n");
        return NULL;
    }

    printf("[suivi] Lancement du suivi de trajectoire (%d points)", traj.nb_points);

    while (1) {
        if (get_position(&pos) != 0) {
            usleep(10000);
            continue;
        }

        // Recherche du point courant dans la trajectoire
        int idx = 1;
        for (int i = 1; i < traj.nb_points - 1; i++) {
            float dx = pos.x - traj.points[i].x;
            float dy = pos.y - traj.points[i].y;
            float dist = sqrtf(dx*dx + dy*dy);
            if (dist < 120.0f) { idx = i; break; }
        }
        if (idx >= traj.nb_points - 1) idx = traj.nb_points - 2;

        // Calcul du polynôme local
        float coeffs[4];
        compute_local_polynomial(&traj.points[idx - 1], &traj.points[idx],
                                 &traj.points[idx + 1], coeffs);

        // Calcul des erreurs
        float d, theta_e;
        compute_errors(pos.x, pos.y, pos.theta, coeffs, &d, &theta_e);

        // Calcul commande
        float v_ref = traj.vitesse;
        float omega = compute_omega(v_ref, d, theta_e);

        // Conversion en vitesses de roues
        float v_left, v_right;
        compute_wheel_speeds(v_ref, omega, &v_left, &v_right);

        // Application moteur
        set_motor_speed(v_left, v_right);

        printf("[suivi] x=%.1f y=%.1f d=%.1f th_e=%.2fdeg ω=%.3f VL=%.1f VR=%.1f\n",
               pos.x, pos.y, d, theta_e*180.0f/PI, omega, v_left, v_right);

        usleep(50000); // 20 Hz
    }

    return NULL;
}
