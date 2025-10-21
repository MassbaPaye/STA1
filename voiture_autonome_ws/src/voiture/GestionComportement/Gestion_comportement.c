#include<stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include"messages.h"
#include"voiture_globals.h"

#define TAG "traj_generator"

static inline long long dist2_point_pos(const Point* p, const PositionVoiture* pos) {
    long long dx = (long long)p->x - (long long)pos->x;
    long long dy = (long long)p->y - (long long)pos->y;
    long long dz = (long long)p->z - (long long)pos->z;
    return dx*dx + dy*dy + dz*dz;
}

static inline float compute_vitesse_convergence(const PositionVoiture* pos) {
    float v = sqrtf(pos->vx  pos->vx + pos->vy  pos->vy + pos->vz * pos->vz);
    if (v > (float)MAX_VITESSE){
        return (float)MAX_VITESSE;
    }
    return v;
}

int generate_trajectoire(int blocking) {
    PositionVoiture pos;
    Itineraire iti;
    DonneesDetection Obj_detecte;
    Consigne cons;

    if (get_position(&pos, blocking) != 0) {
        DBG(TAG, "Failure dans l'obtention de le position");
        return -1;
    }

    if (get_itineraire(&iti, blocking) != 0) {
        DBG(TAG, "Failure dans l'obtention de l'itineraire");
        return -1;
    }

    if (iti.nb_points <= 0 || iti.points == NULL) {
        DBG(TAG, "L'itineraire est vide");
        return -1;
    }

    if(get_donnees_detection(&Obj_detecte, blocking) != 0){
        DBG(TAG, "Failure dans l'obtention des donnes de detection"); 
        return -1;
    }

    if(get_consigne(&cons, blocking) != 0){
        DBG(TAG, "Failure dans l'obtention de la consigne"); 
        return -1;
    }

    int best_idx = 0;
    long long best_d2 = dist2_point_pos(&iti.points[0], &pos);
    float v_current = compute_vitesse_convergence(&pos);
    int point_arret = iti.nb_points;
    int check_obstacle = 0;
    for(int i=0; i<Obj_detecte.nb_panneaux; i++){
        int point_panneau[i] = iti.nb_points;
        int check_panneau[i] = 0;
    }
    for (int i = 1; i < iti.nb_points; ++i) {
        long long d2 = dist2_point_pos(&iti.points[i], &pos);
        if (d2 < best_d2) {
            best_d2 = d2;
            best_idx = i;
        }
        if(Obj_detecte.obstacle_present){
            long long dist_obs2 = (long long)Obj_detecte.obstacle.distance * Obj_detecte.obstacle.distance;
            if(d2 > dist_obs2 && check_obstacle == 0){
                if(Obj_detecte.obstacle.pointd )
                point_arret = i - 1;
                check_obstacle = 1;
            }
            for(int p=0; p<Obj_detecte.nb_panneaux; p++){
                long long dist_pan2[p] = (long long)Obj_detecte.panneaux.distance[p] * Obj_detecte.panneaux.distance[p];
                if(d2 > dist_pan2[p] && check_panneau[p] == 0){
                    point_panneau[p] = i - 1;
                    check_panneau[p] = 1;
                }
            }
        }
    }

    Trajectoire traj;
    memset(&traj, 0, sizeof(traj));
    traj.nb_points = 0;
    traj.vitesse = v_current;
    traj.arreter_fin = false;

    const long long DIST_INSERT_POSITION = (long long)400 * (long long)400; // 40cm^2
    int write_idx = 0;

    if (best_d2 > DIST_INSERT_POSITION) {
        Point pcur;
        pcur.x = pos.x;
        pcur.y = pos.y;
        pcur.z = pos.z;
        pcur.theta = pos.theta;
        traj.points[write_idx++] = pcur;
        traj.nb_points++;
    }

    int start = best_idx + 1;
    for (int i = start; i < iti.nb_points && traj.nb_points < MAX_POINTS_TRAJECTOIRE; i++) {
        traj.points[traj.nb_points++] = iti.points[i];
        for(int p=0; p<Obj_detecte.nb_panneaux; p++){
            if(i == point_panneau[p]){
                switch (Obj_detecte.panneaux->type){
                case 1:
                    traj.vitesse_max = 30;
                    traj.vitesse = 30;
                    i = point_arret;
                    break;
                case 2:
                    traj.vitesse = v_current/2;
                    i = point_arret;
                    break;
                case 3:
                    if(cons.autorisation == 0){
                        break;
                    }
                    Demande d;
                    d.type = 1;
                    set_demande(&d, blocking);
                    i = point_arret;
                    break;
                case 4:
                    traj.vitesse_max = (float)MAX_VITESSE;
                    break;
                }
            }
        }
        if(i == point_arret){
            traj.arreter_fin = true;
            break;
        }
    }

    if (traj.nb_points == 0) {
        traj.points[0] = iti.points[best_idx];
        traj.nb_points = 1;
    }

    if (set_trajectoire(&traj, blocking) != 0) {
        DBG(TAG, "Failure dans definition de la trajectoire");
        return -1;
    }

    DBG(TAG, "Trajectoire gerée avec %d points (best_idx=%d, best_d2=%lld)", traj.nb_points, best_idx, best_d2);
    return 0;
}