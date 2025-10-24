#include "utils.h"
#include "suivi_trajectoire.h"
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
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

bool running_traj = false;
float theta_e;      // Erreur entre l'angle de la voiture et l'angle de la trajectoire
float d;            // Distance entre la voiture et son projetté orthogonal sur la trajectoire
float omega_ref;
float v_ref;
struct timespec last_lost_warn = {0};


// Loi de commande
float compute_omega(float v_ref) {
    return -(v_ref / L1) * tanf(theta_e) - K0 * v_ref * d;
}
// En voie un ordre en utilisant les variables globales v_ref et omega_ref
void send_order() {
    float v_left =  v_ref - ECARTEMENT_ROUE * omega_ref;
    float v_right =  v_ref + ECARTEMENT_ROUE * omega_ref;
    send_motor_speed(v_left, v_right);
}

void send_order_stop() {
    v_ref = 0;
    send_order();
}


void compute_frenet_coordinates_using_closest_point_only(PositionVoiture voiture, Point p) {
    d = - (voiture.x - p.x) * cos(p.theta) + (voiture.y - p.y) * sin(p.theta);
    theta_e = voiture.theta - p.theta; 
}

void update_consignes_closest_point_only(PositionVoiture voiture, Trajectoire traj) {
    int closest_point_id = find_closest_point(voiture, traj);
    compute_frenet_coordinates_using_closest_point_only(voiture, traj.points[closest_point_id]);
    omega_ref = compute_omega(traj.vitesse);
    send_order();
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
    theta_e = voiture.theta - p_proj_abs.theta; // Ou inverse
    d = distance_from_car(voiture, p_proj_abs);
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
void update_consignes_newton(PositionVoiture voiture, Trajectoire traj) {
    int closest_point_id;
    Point closest_point;
    Point p_previous;
    Point p_next;
    closest_point_id = find_closest_point(voiture, traj);
    closest_point = traj.points[closest_point_id];
    float dist_from_closest = distance_from_car(voiture, closest_point);

    if (closest_point_id == traj.nb_points-1 && is_point_overtaken(voiture, traj.points[closest_point_id])) {
        send_order_stop();
        WARN(TAG, "Trajectoire dépassée (distance du dernier point = %1.f)", dist_from_closest);
        return;
    }
    // Calcul de p_previous et p_next
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
        } else{
            // Cas général
            if (is_point_overtaken(voiture, closest_point)) {
                p_previous = closest_point;
                p_next = traj.points[closest_point_id+1];
            } else {
                p_previous = traj.points[closest_point_id-1];
                p_next = closest_point;
            }
        }
    }

    Polynome poly = compute_relative_polynome(to_relative(p_next, voiture));
    Point p_projette_abs = compute_projection_using_l1(voiture, p_previous, poly);
    compute_errors(voiture, p_projette_abs);
    omega_ref = compute_omega(traj.vitesse);
    send_order();
}



// ========== Thread de suivi de trajectoire ==========

void* lancer_suivi_trajectoire(void* arg) {
    (void)arg;
    while(running_traj) {
        Trajectoire traj; 
        PositionVoiture voiture;
        get_trajectoire(&traj);
        update_consignes_newton(voiture, traj);
        update_consignes_closest_point_only(voiture, traj);

        my_sleep(1.0f/UPDATE_MOTOR_FREQ_HZ);
    }
    return NULL;
}