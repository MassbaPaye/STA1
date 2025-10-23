#include<stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#include"messages.h"
#include"utils.h"
#include"config.h"
#include"logger.h"
#include"voiture_globals.h"
#include"Reception_itineraire.h"

#define TAG "traj_generator"

static inline long long dist2_point_pos(const Point* p, const PositionVoiture* pos) {
    long long dx = (long long)p->x - (long long)pos->x;
    long long dy = (long long)p->y - (long long)pos->y;
    long long dz = (long long)p->z - (long long)pos->z;
    return dx*dx + dy*dy + dz*dz;
}

static inline float compute_vitesse_convergence(const PositionVoiture* pos) {
    float v = sqrtf(pos->vx * pos->vx + pos->vy * pos->vy + pos->vz * pos->vz);
    if (v > (float)MAX_VITESSE){
        return (float)MAX_VITESSE;
    }
    return v;
}

int generate_trajectoire() {
    PositionVoiture pos;
    Itineraire iti;
    DonneesDetection Obj_detecte;
    Consigne cons;

    if (get_position(&pos) != 0) {
        DBG(TAG, "Failure dans l'obtention de le position");
        return -1;
    }

    if (get_itineraire(&iti) != 0) {
        DBG(TAG, "Failure dans l'obtention de l'itineraire");
        return -1;
    }

    if (iti.nb_points <= 0 || iti.points == NULL) {
        DBG(TAG, "L'itineraire est vide");
        return -1;
    }

    if(get_donnees_detection(&Obj_detecte) != 0){
        DBG(TAG, "Failure dans l'obtention des donnes de detection"); 
        return -1;
    }
    
    int best_idx = 0;
    long long best_d2 = dist2_point_pos(&iti.points[0], &pos);
    float v_current = compute_vitesse_convergence(&pos);
    int point_arret[MAX_OBSTACLES_SIMULTANES];
    int check_obstacle[MAX_OBSTACLES_SIMULTANES] = {0}; 
    for (int i = 1; i < iti.nb_points; ++i) {
        long long d2 = dist2_point_pos(&iti.points[i], &pos);
        if (d2 < best_d2) {
            best_d2 = d2;
            best_idx = i;
        }if(Obj_detecte.count !=0){
            for(int j = 1; j<Obj_detecte.count; j++){
                Point p_moyen;
                p_moyen.x = (Obj_detecte.obstacle[j].pointd.x + Obj_detecte.obstacle[j].pointg.x) / 2.0f;
                p_moyen.y = (Obj_detecte.obstacle[j].pointd.y + Obj_detecte.obstacle[j].pointg.y) / 2.0f;
                p_moyen.z = (Obj_detecte.obstacle[j].pointd.z + Obj_detecte.obstacle[j].pointg.z) / 2.0f;
                long long dist_obs2 = dist2_point_pos(&p_moyen, 0);
                if(d2 > dist_obs2 && check_obstacle[j] == 0){
                    point_arret[j] = i - 1;
                    check_obstacle[j] = 1;
                }
            }  
        }
    }

    Trajectoire traj;
    memset(&traj, 0, sizeof(traj));
    traj.nb_points = 0;
    traj.vitesse = v_current;
    traj.arreter_fin = false;

    int DIST_INSERT_POSITION = 50 * 50; 
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

    int start = best_idx - 1;
    if(best_idx == 0){
        start = 0;
    }
    for (int i = start; i < iti.nb_points && traj.nb_points < MAX_POINTS_TRAJECTOIRE; i++) {
        traj.points[traj.nb_points++] = iti.points[i];
        for(int j=0; j<Obj_detecte.count; j++){
            if(i == point_arret[j]){
                switch (Obj_detecte.obstacle[j].type){
                    case OBSTACLE_VOITURE:
                        if(iti.points[i].dep == 0){
                            i = MAX_POINTS_TRAJECTOIRE;
                            traj.arreter_fin = true;
                        }else{
                            if(Obj_detecte.obstacle[j].pointd)
                        }
                        break;
                    case PANNEAU_LIMITATION_30:
                        traj.vitesse_max = 30;
                        traj.vitesse = 30;
                        break;
                    case PANNEAU_BARRIERE:
                        i = MAX_POINTS_TRAJECTOIRE;
                        traj.arreter_fin = true;
                        break;
                    case PANNEAU_CEDER_PASSAGE:
                        i = MAX_POINTS_TRAJECTOIRE;
                        traj.arreter_fin = true;
                        break;
                    case PANNEAU_FIN_30:
                        traj.vitesse_max = (float)MAX_VITESSE;
                        break;
                    case PONT:
                        do{
                            i = MAX_POINTS_TRAJECTOIRE;
                            traj.arreter_fin = true;
                            Demande d;
                            d.type = RESERVATION_STRUCTURE;
                            set_demande(&d);
                            usleep(100000);
                            if(get_consigne(&cons) != 0){
                                DBG(TAG, "Failure dans l'obtention de la consigne"); 
                                return -1;
                            }
                            if(pos.vx != 0 || pos.vy != 0){
                                break;
                            }
                        }while(cons.autorisation == CONSIGNE_ATTENTE);
                        break;
                    default:

                        break;
                }
            }
        }
        if(iti.points[i-1].pont == 1 && iti.points[i].pont == 0){
            d.type = LIBERATION_STRUCTURE;
        }
    }
    
 
    if (traj.nb_points == 0) {
        traj.points[0] = iti.points[best_idx];
        traj.nb_points = 1;
    }

    if (set_trajectoire(&traj) != 0) {
        DBG(TAG, "Failure dans definition de la trajectoire");
        return -1;
    }

    DBG(TAG, "Trajectoire gerée avec %d points (best_idx=%d)", traj.nb_points, best_idx);
    return 0;
}

static volatile bool continuer_execution = true;


void* lancer_comportement(void* arg) {
    reception_itineraire();

    printf("[%s] Démarrage du système de génération de trajectoire...\n", TAG);
    
    while (continuer_execution) {
        Itineraire iti;

        int ret = get_itineraire(&iti);

        if (ret != 0 || iti.nb_points <= 0 || iti.points == NULL) {
            printf("[%s] Aucun itinéraire disponible. Nouvelle tentative dans 0.5s...\n", TAG);
            usleep(500000); // attend 500 ms avant de réessayer
            continue;
        }

        int gen_ret = generate_trajectoire(0);
        if (gen_ret != 0) {
            printf("[%s] Erreur lors de la génération de la trajectoire (code=%d)\n", TAG, gen_ret);
        } else {
            printf("[%s] Trajectoire générée avec succès.\n", TAG);
        }

        Trajectoire traj;
        if (get_trajectoire(&traj) == 0) {
            int nb_afficher = traj.nb_points < 5 ? traj.nb_points : 5;
            printf("[%s] --- Aperçu des %d premiers points ---\n", TAG, nb_afficher);

            for (int i = 0; i < nb_afficher; i++) {
                printf("   Point %d -> x=%.2f, y=%.2f, z=%.2f, θ=%.2f\n",
                        i, traj.points[i].x, traj.points[i].y, traj.points[i].z, traj.points[i].theta);
            }

            printf("[%s] -------------------------------------\n", TAG);
        } else {
            printf("[%s] Impossible de récupérer la trajectoire.\n", TAG);
        }
        // Attente avant la prochaine mise à jour (ici : 100 ms)
        usleep(100000);
    }

    printf("[%s] Fin du programme.\n", TAG);
    return 0;
}

void stop_comportement() {
    continuer_execution = false;
}