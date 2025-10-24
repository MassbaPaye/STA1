#include "utils.h"
#include "suivi_trajectoire.h"
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h> // pour usleep()
#include "config.h"
#include "logger.h"
#include "messages.h"
#include "control_tools.h"
#include "communication_serie.h"

#define TAG "suivi-traj"

#define UPDATE_MOTOR_FREQ_HZ 10
#define MAX_NEWTON_ITER 3
#define MIN_DELAY_BEETWEEN_LOST_WARNS_S 1

#define DEG2RAD(x) ((x) * PI / 180.0f)
#define RAD2DEG(x) ((x) * 180.0f / PI)

bool running = false;
float theta_e;      // Erreur entre l'angle de la voiture et l'angle de la trajectoire
float d;            // Distance entre la voiture et son projetté orthogonal sur la trajectoire
float omega_ref;
float v_ref;
struct timespec last_lost_warn = {0};

void* lancer_suivi_trajectoire(void* arg) {
    (void)arg;
    while(running) {
        Trajectoire traj; 
        PositionVoiture voiture; 
        get_trajectoire(&traj);
        int closest_point_id = find_closest_point(voiture, traj);
        compute_frenet_coordinates_using_closest_point_only(voiture, traj.points[closest_point_id]);
        float omega = compute_omega(traj.vitesse, d, theta_e);
    }
    return NULL;
}

void compute_frenet_coordinates_using_closest_point_only(PositionVoiture voiture, Point p) {
    d = - (voiture.x - p.x) * cos(p.theta) + (voiture.y - p.y) * sin(p.theta);
    theta_e = voiture.theta - p.theta; 
}


// Loi de commande
static float compute_omega(float v_ref, float d, float theta_e) {
    return -(v_ref / L1) * tanf(theta_e) - K0 * v_ref * d;
}















// Calcul du projeté orthogonal par la méthode de Newton. On veut annuler un produit scalaire entre la dérivée du polynome et le vecteur PPs 
Point compute_projection_using_l1(PositionVoiture voiture, Point p1_abs, Polynome poly) {
    
    Point ps;
    float dtheta = voiture.theta - p1_abs.theta;
    float xp_rel = voiture.x - p1_abs.x + cosf(dtheta)*L1;
    float yp_rel = voiture.y - p1_abs.y + sinf(dtheta)*L1;
    float x_s = xp_rel;

    for (int i = 0; i < MAX_NEWTON_ITER; ++i) {
        float y_s = poly.a0 + poly.a1*x_s + poly.a2*x_s*x_s + poly.a3*x_s*x_s*x_s;
        float dy_dx = poly.a1 + 2.0f*poly.a2*x_s + 3.0f*poly.a3*x_s*x_s;
        float f = (xp_rel - x_s) + (y_s - yp_rel)*dy_dx;
        float df = -1.0f + dy_dx*dy_dx + (y_s - yp_rel)*(2.0f*poly.a2 + 6.0f*poly.a3*x_s);
        if (fabsf(df) < 1.0) break;
        x_s -= f / df;
    }
    float y_s = poly.a0 + poly.a1*x_s + poly.a2*x_s*x_s + poly.a3*x_s*x_s*x_s;
    ps.x = x_s;
    ps.y = y_s;
    ps.z = 0;
    ps.theta = atanf(poly.a1 + 2.0f*poly.a2*x_s + 3.0f*poly.a3*x_s*x_s);
    
    return to_absolute(ps, voiture);
}

void compute_errors(PositionVoiture voiture, Point p_proj_abs) {
    
}

Point move_linear_point(Point p, float distance) {
    Point p_moved;
    p_moved.x = p.x + cosf(p.theta)*distance;
    p_moved.y = p.y + sinf(p.theta)*distance;
    p_moved.z = 0;
    p_moved.theta = p.theta;
    return p_moved;
}



// Calcul des erreurs latérale et angulaire
static void update_consignes(PositionVoiture voiture, Trajectoire traj) {
    int closest_point_id;
    Point closest_point;
    Point p_previous;
    Point p_next;
    closest_point_id = find_closest_point(voiture, traj);
    closest_point = traj.points[closest_point_id];
    float dist_from_closest = distance_from_car(voiture, closest_point);

    if (closest_point_id == traj.nb_points-1 && is_point_overtaken(voiture, traj.points[closest_point_id])) {
        car_order_stop();
        WARN(TAG, "Trajectoire dépassée (distance du dernier point = %1.f)", dist_from_closest);
        return;
    }
    if (closest_point_id == 0 && !is_point_overtaken(voiture, traj.points[closest_point_id])) {
        p_next = closest_point;
        p_previous = move_linear_point(closest_point, -dist_from_closest);
        WARN(TAG, "Trajectoire fournie trop en avance sur la position (distance du premier point = %1.f)", dist_from_closest);
    } else {
    if (dist_from_closest > MAX_TRAJ_OFFSET) {
        struct timespec t_now;
        clock_gettime(CLOCK_MONOTONIC, &t_now);
        if (timespec_diff_s(last_lost_warn, t_now) > MIN_DELAY_BEETWEEN_LOST_WARNS_S)
            WARN(TAG, "Voiture perdue, plus proche point de la traj à %1.f mm", dist_from_closest);
    }
    // TODO
    // Calculer p_previous p_next
    if (is_point_overtaken(voiture, closest_point)) {

    }
    compute_relative_polynome(to_relative());
    compute_projection_using_l1(voiture, closest_point, poly)
    float dy_dx = coeffs[1] + 2*coeffs[2]*xs + 3*coeffs[3]*xs*xs;
    float theta_s = atan(dy_dx);
    float nx = -sin(theta_s), ny = cos(theta_s);
    *d = (x - xs)*nx + (y - ys)*ny;
    float theta_r = DEG2RAD(theta_deg);
    *theta_e = theta_r - theta_s;
    while (*theta_e > PI) *theta_e -= 2*PI;
    while (*theta_e < -PI) *theta_e += 2*PI;
}

void send_order_stop() {
    v_ref = 0;
    send_order();
}

// En voie un ordre en utilisant les variables globales v_ref et omega_ref
void send_order() {
    float v_left =  v_ref - ECARTEMENT_ROUE * omega_ref;
    float v_right =  v_ref + ECARTEMENT_ROUE * omega_ref;
    envoyer_consigne_moteur((int) v_left, (int) v_right);
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
