#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>
#include "main_suivi_traj.h"
#include "logger.h"
#include "communication_serie.h"
#include "main_localisation.h"
#include "utils.h"
#include "config.h"

#define TAG "TRAJECTOIRE_SPLINE"

// === PARAMÈTRES PHYSIQUES ===
#define RAYON_ROUE_MM 55.0
#define DEMI_VOIE_MM  170.0
#define DISTANCE_SEUIL_MM 5.0
#define FREQUENCE_HZ  20.0 // fréquence de mise à jour

// === GAINS METHODE SAMSON (PAPIER) ===
#define KP1 1.0   // k1
#define KP2 1.0   // k2
#define KP3 3.0   // k3

// === GAINS SECOURS (Samson classique si |θe| trop grand) ===
#define K1 1.0
#define K2 2.0
#define K3 1.0

// === VARIABLES GLOBALES ===
static pthread_t thread_trajectoire;
static volatile bool trajectoire_active = false;
static Trajectoire* trajectoire_actuelle = NULL;

// === FONCTIONS INTERNES ===
static void* thread_suivi_trajectoire(void* arg);
static void envoyer_consigne(double v, double omega);
static double wrap_angle(double a);
static void catmull_rom(double t, Point p0, Point p1, Point p2, Point p3,
                        double* x, double* y, double* dx, double* dy, double* ddx, double* ddy);

// ============================================================================
// Vérifie si trajectoire active
// ============================================================================
bool est_trajectoire_active(void) {
    return trajectoire_active;
}

// ============================================================================
// Démarrage du suivi spline + Samson (méthode papier)
// ============================================================================
void demarrer_trajectoire(Trajectoire* traj) {
    if (trajectoire_active) {
        WARN(TAG, "Une trajectoire est déjà en cours !");
        return;
    }

    if (!traj || traj->nb_points < 4) {
        ERR(TAG, "Trajectoire invalide (au moins 4 points nécessaires pour spline)");
        return;
    }

    trajectoire_actuelle = traj;
    trajectoire_active = true;

    if (pthread_create(&thread_trajectoire, NULL, thread_suivi_trajectoire, NULL) != 0) {
        perror("Erreur pthread_create trajectoire");
        trajectoire_active = false;
    } else {
        INFO(TAG, "Thread de trajectoire (Spline + Samson-Papier) lancé (%d points)", traj->nb_points);
    }
}

// ============================================================================
// Arrêt propre
// ============================================================================
void arreter_trajectoire(void) {
    if (!trajectoire_active) {
        WARN(TAG, "Aucune trajectoire à arrêter");
        return;
    }
    trajectoire_active = false;
    pthread_join(thread_trajectoire, NULL);
    envoyer_consigne(0, 0);
    INFO(TAG, "Trajectoire stoppée proprement.");
}

// ============================================================================
// Thread principal : suivi spline + Samson (méthode du papier)
// ============================================================================
static void* thread_suivi_trajectoire(void* arg) {
    Trajectoire* traj = trajectoire_actuelle;
    double dt = 1.0 / FREQUENCE_HZ;
    double s = 0.0;
    int idx = 1;

    INFO(TAG, "Suivi de trajectoire spline + Samson (méthode papier) démarré.");

    while (trajectoire_active && idx < traj->nb_points - 2) {
        Point p0 = traj->points[idx - 1];
        Point p1 = traj->points[idx];
        Point p2 = traj->points[idx + 1];
        Point p3 = traj->points[idx + 2];

        double t = s;
        if (t > 1.0) {
            t = 0.0;
            s = 0.0;
            idx++;
            continue;
        }

        // --- Évaluer spline et dérivées ---
        double x_r, y_r, dx_r, dy_r, ddx_r, ddy_r;
        catmull_rom(t, p0, p1, p2, p3, &x_r, &y_r, &dx_r, &dy_r, &ddx_r, &ddy_r);

        // Conversion mm -> m
        x_r /= 1000.0; y_r /= 1000.0;
        dx_r /= 1000.0; dy_r /= 1000.0;
        ddx_r /= 1000.0; ddy_r /= 1000.0;

        // Orientation et courbure
        double theta_r = atan2(dy_r, dx_r);
        double v_r = traj->vitesse / 1000.0;
        double kappa = (dx_r * ddy_r - dy_r * ddx_r) / pow(dx_r * dx_r + dy_r * dy_r, 1.5);
        double omega_r = kappa * v_r;

        // --- Lecture position réelle ---
        PositionVoiture pos;
        get_position(&pos);
        double x = pos.x / 1000.0;
        double y = pos.y / 1000.0;
        double theta = pos.theta * PI / 180.0;

        // --- Erreurs dans le repère du véhicule de référence ---
        double dxg = x - x_r;
        double dyg = y - y_r;

        double xe =  cos(theta_r) * dxg + sin(theta_r) * dyg;
        double ye = -sin(theta_r) * dxg + cos(theta_r) * dyg;
        double theta_e = wrap_angle((theta - theta_r) * 180.0 / M_PI) * M_PI / 180.0;

        // --- Application de la méthode du papier ---
        const double ANGLE_SEUIL_RAD = 1.2; // bascule sur loi classique si > ~70°

        double v_cmd = 0.0;
        double omega_cmd = 0.0;

        if (fabs(theta_e) > ANGLE_SEUIL_RAD) {
            // Loi Samson classique (sécurité)
            double e_x =  cos(theta) * (x_r - x) + sin(theta) * (y_r - y);
            double e_y = -sin(theta) * (x_r - x) + cos(theta) * (y_r - y);
            double e_th = wrap_angle((theta_r - theta) * 180.0 / M_PI) * M_PI / 180.0;

            v_cmd = v_r * cos(e_th) + K1 * e_x;
            omega_cmd = omega_r + K2 * v_r * e_y + K3 * sin(e_th);
        } else {
            // Méthode Samson (papier - Proposition 4)
            double u1r = v_r;
            double u2r = omega_r;

            double z1 = xe;
            double z2 = ye;
            double z3 = tan(theta_e);

            double w1 = -KP1 * fabs(u1r) * (z1); // version linéarisée
            double w2 = -KP2 * u1r * z2 - KP3 * fabs(u1r) * z3;

            double cos_te = cos(theta_e);
            double cos_te2 = cos_te * cos_te;

            double u1 = (w1 + u1r) / cos_te;
            double u2 = w2 * cos_te2 + u2r;

            v_cmd = u1;
            omega_cmd = u2;
        }

        // --- Saturations physiques ---
        double v_max = traj->vitesse_max / 1000.0;
        if (fabs(v_cmd) > v_max) v_cmd = copysign(v_max, v_cmd);

        const double OMEGA_MAX = 3.0;
        if (fabs(omega_cmd) > OMEGA_MAX) omega_cmd = copysign(OMEGA_MAX, omega_cmd);

        // --- Envoi des consignes ---
        envoyer_consigne(v_cmd, omega_cmd);

        // Avancement sur la spline
        s += (v_r * dt) / sqrt(dx_r * dx_r + dy_r * dy_r);

        usleep((useconds_t)(dt * 1e6));
    }

    envoyer_consigne(0, 0);
    trajectoire_active = false;
    INFO(TAG, "Trajectoire spline terminée.");
    pthread_exit(NULL);
}

// ============================================================================
// Catmull-Rom spline interpolation
// ============================================================================
static void catmull_rom(double t, Point p0, Point p1, Point p2, Point p3,
                        double* x, double* y, double* dx, double* dy, double* ddx, double* ddy) {
    double t2 = t * t;
    double t3 = t2 * t;

    *x = 0.5 * ((2*p1.x) +
                (-p0.x + p2.x)*t +
                (2*p0.x - 5*p1.x + 4*p2.x - p3.x)*t2 +
                (-p0.x + 3*p1.x - 3*p2.x + p3.x)*t3);
    *y = 0.5 * ((2*p1.y) +
                (-p0.y + p2.y)*t +
                (2*p0.y - 5*p1.y + 4*p2.y - p3.y)*t2 +
                (-p0.y + 3*p1.y - 3*p2.y + p3.y)*t3);

    *dx = 0.5 * ((-p0.x + p2.x) +
                2*(2*p0.x - 5*p1.x + 4*p2.x - p3.x)*t +
                3*(-p0.x + 3*p1.x - 3*p2.x + p3.x)*t2);
    *dy = 0.5 * ((-p0.y + p2.y) +
                2*(2*p0.y - 5*p1.y + 4*p2.y - p3.y)*t +
                3*(-p0.y + 3*p1.y - 3*p2.y + p3.y)*t2);

    *ddx = 0.5 * (2*(2*p0.x - 5*p1.x + 4*p2.x - p3.x) +
                 6*(-p0.x + 3*p1.x - 3*p2.x + p3.x)*t);
    *ddy = 0.5 * (2*(2*p0.y - 5*p1.y + 4*p2.y - p3.y) +
                 6*(-p0.y + 3*p1.y - 3*p2.y + p3.y)*t);
}

// ============================================================================
// Conversion (v, ω) → (ωR, ωL) puis envoi vers MegaPi
// ============================================================================
static void envoyer_consigne(double v, double omega) {
    double r = RAYON_ROUE_MM / 1000.0;
    double b = DEMI_VOIE_MM / 1000.0;

    double wR = (v + b * omega) / r;
    double wL = (v - b * omega) / r;

    envoyer_consigne_moteur((int)wR, (int)wL);
}

// ============================================================================
// Normalisation d'angle [-180,180]
// ============================================================================
static double wrap_angle(double a) {
    while (a > 180) a -= 360;
    while (a < -180) a += 360;
    return a;
}
