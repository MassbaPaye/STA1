#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>
#include "logger.h"
#include "main_trajectoire.h"
#include "communication_serie.h"
#include "main_localisation.h"

#define TAG "TRAJECTOIRE_SPLINE"

// === PARAMÈTRES PHYSIQUES ===
#define RAYON_ROUE_MM 50.0
#define DEMI_VOIE_MM  200.0
#define DISTANCE_SEUIL_MM 100.0
#define FREQUENCE_HZ  20.0 // plus fluide

// === GAINS SAMSON ===
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

// Catmull-Rom interpolation (t ∈ [0,1])
static void catmull_rom(double t, Point p0, Point p1, Point p2, Point p3,
                        double* x, double* y, double* dx, double* dy, double* ddx, double* ddy);

// ============================================================================
// Vérifie si trajectoire active
// ============================================================================
bool est_trajectoire_active(void) {
    return trajectoire_active;
}

// ============================================================================
// Démarrage du suivi spline + Samson
// ============================================================================
void demarrer_trajectoire(Trajectoire* traj) {
    if (trajectoire_active) {
        WARN(TAG, "Une trajectoire est déjà en cours !");
        return;
    }

    if (!traj || traj->nb_points < 4) {
        ERROR(TAG, "Trajectoire invalide (au moins 4 points nécessaires pour spline)");
        return;
    }

    trajectoire_actuelle = traj;
    trajectoire_active = true;

    if (pthread_create(&thread_trajectoire, NULL, thread_suivi_trajectoire, NULL) != 0) {
        perror("Erreur pthread_create trajectoire");
        trajectoire_active = false;
    } else {
        INFO(TAG, "Thread de trajectoire (Spline + Samson) lancé (%d points)", traj->nb_points);
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
// Thread principal : suivi spline + Samson
// ============================================================================
static void* thread_suivi_trajectoire(void* arg) {
    Trajectoire* traj = trajectoire_actuelle;
    double dt = 1.0 / FREQUENCE_HZ;
    double s = 0.0;
    int idx = 1; // commence à partir du 2e point (p0,p1,p2,p3)

    INFO(TAG, "Suivi de trajectoire spline + Samson démarré.");

    while (trajectoire_active && idx < traj->nb_points - 2) {
        Point p0 = traj->points[idx - 1];
        Point p1 = traj->points[idx];
        Point p2 = traj->points[idx + 1];
        Point p3 = traj->points[idx + 2];

        double t = s; // t évolue entre [0,1] le long du segment
        if (t > 1.0) {
            t = 0.0;
            s = 0.0;
            idx++;
            continue;
        }

        // --- Évaluer spline et ses dérivées ---
        double x_r, y_r, dx_r, dy_r, ddx_r, ddy_r;
        catmull_rom(t, p0, p1, p2, p3, &x_r, &y_r, &dx_r, &dy_r, &ddx_r, &ddy_r);

        // Convertir en m
        x_r /= 1000.0;
        y_r /= 1000.0;
        dx_r /= 1000.0;
        dy_r /= 1000.0;
        ddx_r /= 1000.0;
        ddy_r /= 1000.0;

        // --- Calcul orientation et courbure ---
        double theta_r = atan2(dy_r, dx_r);
        double v_r = traj->vitesse / 1000.0;
        double kappa = (dx_r * ddy_r - dy_r * ddx_r) / pow(dx_r * dx_r + dy_r * dy_r, 1.5);
        double omega_r = kappa * v_r;

        // --- Lecture position réelle ---
        double x = get_position_x() / 1000.0;
        double y = get_position_y() / 1000.0;
        double theta = get_orientation() * M_PI / 180.0;

        // --- Calcul erreurs (repère robot) ---
        double dx = x_r - x;
        double dy = y_r - y;
        double e_theta = wrap_angle((theta_r - theta) * 180.0 / M_PI) * M_PI / 180.0;
        double e_x =  cos(theta) * dx + sin(theta) * dy;
        double e_y = -sin(theta) * dx + cos(theta) * dy;

        // --- Loi de Samson ---
        double v = v_r * cos(e_theta) + K1 * e_x;
        double omega = omega_r + K2 * v_r * e_y + K3 * sin(e_theta);

        // Limiter v
        double v_max = traj->vitesse_max / 1000.0;
        if (fabs(v) > v_max) v = copysign(v_max, v);

        envoyer_consigne(v, omega);

        // Avancer sur la spline (paramètre t)
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

    // Coefficients (centripète standard, alpha=0.5)
    *x = 0.5 * ((2*p1.x) +
                (-p0.x + p2.x)*t +
                (2*p0.x - 5*p1.x + 4*p2.x - p3.x)*t2 +
                (-p0.x + 3*p1.x - 3*p2.x + p3.x)*t3);
    *y = 0.5 * ((2*p1.y) +
                (-p0.y + p2.y)*t +
                (2*p0.y - 5*p1.y + 4*p2.y - p3.y)*t2 +
                (-p0.y + 3*p1.y - 3*p2.y + p3.y)*t3);

    // Dérivées 1ère et 2e
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
// Conversion v,ω → ωR,ωL
// ============================================================================
static void envoyer_consigne(double v, double omega) {
    double r = RAYON_ROUE_MM / 1000.0;
    double b = DEMI_VOIE_MM / 1000.0;
    double wR = (v + b * omega) / r;
    double wL = (v - b * omega) / r;
    envoyer_vitesses_chenilles(wR, wL);
}

// ============================================================================
// Normalisation d'angle [-180,180]
// ============================================================================
static double wrap_angle(double a) {
    while (a > 180) a -= 360;
    while (a < -180) a += 360;
    return a;
}
