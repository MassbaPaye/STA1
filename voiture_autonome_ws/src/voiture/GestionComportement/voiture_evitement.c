// Fichier : voiture_evitement.c
// Module minimal de réaction locale : stop / ralentir / contournement dans la voie
// Dépendances : messages.h, voiture_globals.h

#include <math.h>
#include <string.h>
#include <stdbool.h>
#include "messages.h"
#include "voiture_globals.h"

// -------------------- Paramètres simples (mm / mm.s-1) --------------------
#define VITESSE_EVITEMENT   10    // mm/s
#define LONGUEUR_VOITURE   240    // mm
#define LARGEUR_VOITURE    140    // mm
#define SECURITY_MARGE      50    // mm
#define DISTANCE_STOP      100    // mm
#define NB_PTS_TRAJ          4

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// -------------------- Petites aides math --------------------
static inline float deg2rad(float d) { return d * (float)M_PI / 180.0f; }
static inline float hypot2f(float dx, float dy){ return sqrtf(dx*dx + dy*dy); }

// ============================================================================
// Fonction : lire_donnees_capteurs
// Rôle     : Lire les données nécessaires (détection & position) via les globals.
// Retour   : 0 si OK, <0 si indisponible.
// ============================================================================
static int lire_donnees_capteurs(DonneesDetection* det, PositionVoiture* pos) {
    if (get_donnees_detection(det) != 0) return -1;
    if (get_position(pos) != 0)          return -2;
    return 0;
}

// ============================================================================
// Fonction : trajectoire_actuelle_permet_de_passer
// Rôle     : Tester grossièrement si la trajectoire courante passe loin de l'obstacle.
//            Critère simple : tous les points de la spline sont à > marge de l'obstacle.
// Retour   : true si ça passe ; false sinon (il faut réagir).
// Remarque : on mesure par rapport au centre de l'obstacle (moyenne pointg/pointd).
// ============================================================================
static bool trajectoire_actuelle_permet_de_passer(const Obstacle* obs_abs) {
    Trajectoire t;
    if (get_trajectoire(&t) != 0 || t.nb_points <= 0) {
        // Pas de trajectoire: mieux vaut considérer qu'il faut en générer une
        return false;
    }

    const float cx = 0.5f * (obs_abs->pointg.x + obs_abs->pointd.x);
    const float cy = 0.5f * (obs_abs->pointg.y + obs_abs->pointd.y);
    const float clearance = (LARGEUR_VOITURE * 0.5f) + SECURITY_MARGE;

    for (int i = 0; i < t.nb_points; ++i) {
        float dx = (float)t.points[i].x - cx;
        float dy = (float)t.points[i].y - cy;
        if (hypot2f(dx, dy) < clearance) {
            return false; // trop près : ne passe pas
        }
    }
    return true;
}

// ============================================================================
// Fonction : evaluer_distance_obstacle_local
// Rôle     : Décider "stop" ou "ralentir" selon la distance Y locale (repère voiture).
//            Si l'obstacle est derrière (Y<=0), on ne fait rien de spécial.
// Retour   : 0 = STOP, 1 = RALENTIR, 2 = RIEN (obstacle loin/derrière).
// ============================================================================
static int evaluer_distance_obstacle_local(const Obstacle* obs_local) {
    // Centre local de l'obstacle : moyenne de pointg / pointd (repère voiture)
    const float cy_local = 0.5f * (obs_local->pointg.y + obs_local->pointd.y);

    if (cy_local <= 0.0f) return 2;                 // derrière : ignorer
    if (cy_local <  (float)DISTANCE_STOP) return 0; // STOP
    // Entre DISTANCE_STOP et plus loin : ralentir pour préparer l'évitement
    return 1;                                       // RALENTIR
}

// ============================================================================
// Fonction : convertir_obstacle_local_vers_absolu
// Rôle     : Convertir les points de l'obstacle (repère voiture) vers repère global.
// Effet    : Écrase obs_local -> obs_abs (écriture dans *obs_abs).
// ============================================================================
static void convertir_obstacle_local_vers_absolu(const Obstacle* obs_local,
                                                 const PositionVoiture* pos,
                                                 Obstacle* obs_abs)
{
    const float th = deg2rad(pos->theta);
    const float cg = cosf(th), sg = sinf(th);

    // Gauche
    float xg = (float)obs_local->pointg.x;
    float yg = (float)obs_local->pointg.y;
    obs_abs->pointg.x = (int)lroundf(pos->x + xg * cg - yg * sg);
    obs_abs->pointg.y = (int)lroundf(pos->y + xg * sg + yg * cg);
    obs_abs->pointg.z = (int)lroundf(pos->z);
    obs_abs->pointg.theta = pos->theta;

    // Droite
    float xd = (float)obs_local->pointd.x;
    float yd = (float)obs_local->pointd.y;
    obs_abs->pointd.x = (int)lroundf(pos->x + xd * cg - yd * sg);
    obs_abs->pointd.y = (int)lroundf(pos->y + xd * sg + yd * cg);
    obs_abs->pointd.z = (int)lroundf(pos->z);
    obs_abs->pointd.theta = pos->theta;

    obs_abs->type = obs_local->type;
}

// ============================================================================
// Fonction : choisir_cote_contournement
// Rôle     : Choisir le côté à passer (gauche = -1, droite = +1) selon la marge
//            disponible vis-à-vis des bords de voie.
// ============================================================================
static int choisir_cote_contournement(const Obstacle* obs_abs, const MarquageSol* ms) {
    if (ms->nb_points_gauche <= 0 || ms->nb_points_droite <= 0) return +1; // défaut : droite

    Point bordG = ms->ligne_gauche[0];
    Point bordD = ms->ligne_droite[0];

    float mg = hypot2f((float)obs_abs->pointg.x - bordG.x, (float)obs_abs->pointg.y - bordG.y);
    float md = hypot2f((float)obs_abs->pointd.x - bordD.x, (float)obs_abs->pointd.y - bordD.y);

    // Plus de marge du côté gauche -> passer à gauche (-1), sinon à droite (+1)
    return (mg > md) ? -1 : +1;
}

// ============================================================================
// Fonction : contournement_est_possible
// Rôle     : Vérifier qu'il reste au moins SECURITY_MARGE de part et d'autre entre
//            l'obstacle et les bords ; sinon, contournement risqué.
// ============================================================================
static bool contournement_est_possible(const Obstacle* obs_abs, const MarquageSol* ms) {
    if (ms->nb_points_gauche <= 0 || ms->nb_points_droite <= 0) return false;

    Point bordG = ms->ligne_gauche[0];
    Point bordD = ms->ligne_droite[0];

    float mg = hypot2f((float)obs_abs->pointg.x - bordG.x, (float)obs_abs->pointg.y - bordG.y);
    float md = hypot2f((float)obs_abs->pointd.x - bordD.x, (float)obs_abs->pointd.y - bordD.y);

    const float min_clear = SECURITY_MARGE + (LARGEUR_VOITURE * 0.5f);
    return (mg > min_clear && md > min_clear);
}

// ============================================================================
// Fonction : generer_traj_stop_ou_ralentir
// Rôle     : Générer une petite trajectoire de stop (v=0) ou de ralentissement
//            (v=VITESSE_EVITEMENT) dans l'axe courant.
// ============================================================================
static void generer_traj_stop_ou_ralentir(Trajectoire* out,
                                          const PositionVoiture* pos,
                                          bool stop)
{
    memset(out, 0, sizeof(*out));
    const float th = deg2rad(pos->theta);
    const float hx = cosf(th), hy = sinf(th);

    out->nb_points = 2;
    out->points[0].x = (int)lroundf(pos->x);
    out->points[0].y = (int)lroundf(pos->y);
    out->points[0].z = (int)lroundf(pos->z);
    out->points[0].theta = pos->theta;

    out->points[1].x = (int)lroundf(pos->x + 100.0f * hx);
    out->points[1].y = (int)lroundf(pos->y + 100.0f * hy);
    out->points[1].z = (int)lroundf(pos->z);
    out->points[1].theta = pos->theta;

    out->vitesse     = stop ? 0.0f : (float)VITESSE_EVITEMENT;
    out->vitesse_max = stop ? 0.0f : (float)(2 * VITESSE_EVITEMENT);
    out->arreter_fin = stop;
}

// ============================================================================
// Fonction : generer_traj_contournement_dans_voie
// Rôle     : Créer 4 points à l'intérieur du couloir (MarquageSol). On suit
//            grossièrement la ligne centrale et on applique un décalage latéral
//            "offset" vers le côté choisi (cote = -1 gauche, +1 droite).
// ============================================================================
static void generer_traj_contournement_dans_voie(Trajectoire* out,
                                                 const MarquageSol* ms,
                                                 int cote /* -1 gauche, +1 droite */)
{
    memset(out, 0, sizeof(*out));

    // Sécurité : si pas assez de points de marquage, on fait simple et court
    int n = ms->nb_points_gauche;
    if (ms->nb_points_droite < n) n = ms->nb_points_droite;
    if (n <= 0) {
        out->nb_points = 0;
        return;
    }

    out->nb_points = (n >= NB_PTS_TRAJ) ? NB_PTS_TRAJ : n;

    for (int i = 0; i < out->nb_points; ++i) {
        Point g = ms->ligne_gauche[i];
        Point d = ms->ligne_droite[i];

        // centre et vecteur latéral
        float cx = 0.5f * (g.x + d.x);
        float cy = 0.5f * (g.y + d.y);
        float vx = (float)(d.x - g.x), vy = (float)(d.y - g.y);
        float w  = hypot2f(vx, vy);
        if (w < 1e-6f) w = 1.0f;
        vx /= w; vy /= w;

        // offset latéral limité pour rester dans la voie
        float offset_max = 0.5f * w - SECURITY_MARGE - (LARGEUR_VOITURE * 0.5f);
        if (offset_max < 0.0f) offset_max = 0.0f;
        float offset = 0.3f * w;              // décale d'environ 30% de la voie
        if (offset > offset_max) offset = offset_max;
        offset *= (float)cote;                // signe du côté choisi

        out->points[i].x = (int)lroundf(cx + offset * vx);
        out->points[i].y = (int)lroundf(cy + offset * vy);
        out->points[i].z = 0;
        out->points[i].theta = 0.0f;          // simple : orientation non critique pour la spline
    }

    out->vitesse     = (float)VITESSE_EVITEMENT;
    out->vitesse_max = (float)(2 * VITESSE_EVITEMENT);
    out->arreter_fin = false;
}

// ============================================================================
// Fonction : publier_traj
// Rôle     : Publier la trajectoire calculée au suiveur via les globals.
// ============================================================================
static void publier_traj(const Trajectoire* t) {
    if (t && t->nb_points >= 2) {
        (void)set_trajectoire(t);
    }
}

// ============================================================================
// Fonction : voiture_evitement_main
// Rôle     : Orchestration complète :
//            0) si la trajectoire actuelle passe, ne rien faire,
//            1) décider stop/ralentir selon distance locale,
//            2) convertir obstacle en absolu,
//            3) choisir côté & vérifier la faisabilité,
//            4) générer la trajectoire de contournement dans la voie,
//            5) publier.
// Retour   : 0 si OK, <0 si problème / impossibilité.
// ============================================================================
int voiture_evitement_main(void)
{
    DonneesDetection det;
    PositionVoiture  pos;
    if (lire_donnees_capteurs(&det, &pos) < 0) return -1;
    if (det.count <= 0) return 0;  // Pas d'obstacle détecté

    // --- Filtrer pour garder UNIQUEMENT les voitures (OBSTACLE_VOITURE) ---
    // Trouver la voiture la PLUS PROCHE (Y minimal en coordonnées locales)
    int nearest_car_idx = -1;
    float nearest_y = 1e9f;  // Grande valeur initiale
    
    for (int i = 0; i < det.count; i++) {
        // Filtrer: garder uniquement les voitures
        if (det.obstacle[i].type != OBSTACLE_VOITURE) continue;
        
        // Calculer Y local (position devant la voiture)
        float cy = 0.5f * (det.obstacle[i].pointg.y + det.obstacle[i].pointd.y);
        
        // Ignorer les obstacles derrière (Y <= 0)
        if (cy <= 0.0f) continue;
        
        // Garder la plus proche (Y minimal)
        if (cy < nearest_y) {
            nearest_y = cy;
            nearest_car_idx = i;
        }
    }
    
    // Aucune voiture détectée devant nous
    if (nearest_car_idx < 0) return 0;

    // --- Étape 1 : stop / ralentir (avec coordonnées locales IA) ---
    // (On duplique l'obstacle pour garder la version "locale" utile ici)
    Obstacle obs_local = det.obstacle[nearest_car_idx];  // Traiter la voiture la plus proche
    int decision = evaluer_distance_obstacle_local(&obs_local);
    if (decision == 0) {
        Trajectoire t_stop;
        generer_traj_stop_ou_ralentir(&t_stop, &pos, /*stop=*/true);
        publier_traj(&t_stop);
        return 0;
    }
    // decision == 1 -> on ralentit, et on prépare un éventuel contournement
    // decision == 2 -> rien de spécial, mais on peut quand même vérifier 0)

    // --- Étape 2 : convertir obstacle en absolu pour s'aligner avec MarquageSol ---
    Obstacle obs_abs;
    convertir_obstacle_local_vers_absolu(&obs_local, &pos, &obs_abs);

    // --- Étape 0 : si la trajectoire actuelle passe déjà, on ne touche à rien ---
    if (trajectoire_actuelle_permet_de_passer(&obs_abs)) {
        // Optionnel : si on veut quand même ralentir un peu en approche
        if (decision == 1) {
            Trajectoire t_slow;
            generer_traj_stop_ou_ralentir(&t_slow, &pos, /*stop=*/false);
            publier_traj(&t_slow);
        }
        return 0;
    }

    // --- Étape 3 : choisir côté + Étape 4 : vérifier faisabilité ---
    int cote = choisir_cote_contournement(&obs_abs, &det.marquage_sol);
    if (!contournement_est_possible(&obs_abs, &det.marquage_sol)) {
        // Trop étroit -> stop propre
        Trajectoire t_stop;
        generer_traj_stop_ou_ralentir(&t_stop, &pos, /*stop=*/true);
        publier_traj(&t_stop);
        return -2;
    }

    // --- Étape 5 : générer la trajectoire de contournement dans la voie ---
    Trajectoire t_contour;
    generer_traj_contournement_dans_voie(&t_contour, &det.marquage_sol, cote);
    if (t_contour.nb_points < 2) {
        // Marquage indisponible -> prudence : stop
        Trajectoire t_stop;
        generer_traj_stop_ou_ralentir(&t_stop, &pos, /*stop=*/true);
        publier_traj(&t_stop);
        return -3;
    }

    // --- Étape 6 : publier ---
    publier_traj(&t_contour);

    // --- Étape 7 : OK ---
    return 0;
}