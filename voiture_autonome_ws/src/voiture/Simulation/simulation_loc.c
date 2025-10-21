#ifdef SIMULATION

#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "voiture_globals.h"
#include "marvelmind_manager.h"
#include "config.h"
#include "logger.h"

#define TAG "simulateur"
#define MARVELMIND_UPDATE_FREQ_HZ 2
#define SIM_FREQ_HZ 10
#define SIM_DT (1.0 / SIM_FREQ_HZ)

// Paramètres du robot simulé
static float v_gauche = 150.0f;  // mm/s
static float v_droite = 150.0f;  // mm/s
static float angle_deg = 0.0f;
static float x_real = 0.0f, y_real = 0.0f;

// Paramètres d'erreur odométrique
#define ODOMETRIE_BIAIS_GAUCHE  0.02f
#define ODOMETRIE_BIAIS_DROITE -0.01f
#define ODOMETRIE_NOISE_RATIO   0.01f

static inline float rand_sym() {
    return 2.0f * ((float)rand() / RAND_MAX) - 1.0f;
}

void* lancer_simulateur() {
    INFO(TAG, "Simulation capteurs + Marvelmind démarrée");

    // Créer le dossier output s'il n'existe pas
    struct stat st = {0};
    if (stat("output", &st) == -1) {
        if (mkdir("output", 0755) != 0) {
            ERR(TAG, "Impossible de créer le dossier 'output'");
            return NULL;
        }
    }

    srand(time(NULL));
    struct timespec t_now;
    double t_last_mm = 0.0, t_start = 0.0;

    // --- Ouverture du fichier CSV ---
    FILE* f = fopen("output/simulation_log.csv", "w");
    if (!f) {
        ERR(TAG, "Impossible d'ouvrir le fichier CSV !");
        return NULL;
    }

    fprintf(f, "t_s,x_real,y_real,vx,vy,x_fusion,y_fusion,x_mm,y_mm,mm_valid\n");
    fflush(f);

    // Initialisation du temps de référence
    clock_gettime(CLOCK_MONOTONIC, &t_now);
    t_start = t_now.tv_sec + t_now.tv_nsec / 1e9;

    while (1) {
        // 1️⃣ Trajectoire simulée
        angle_deg += ((v_droite - v_gauche) / (2 * ECARTEMENT_ROUE)) * SIM_DT * (180.0f / PI);
        if (angle_deg > 360.0f) angle_deg -= 360.0f;

        float v = (v_droite + v_gauche) / 2.0f;
        x_real += v * cosf(angle_deg * PI / 180.0f) * SIM_DT;
        y_real += v * sinf(angle_deg * PI / 180.0f) * SIM_DT;

        // 2️⃣ Appliquer biais + bruit
        float v_gauche_mesuree = v_gauche * (1.0f + ODOMETRIE_BIAIS_GAUCHE + ODOMETRIE_NOISE_RATIO * rand_sym());
        float v_droite_mesuree = v_droite * (1.0f + ODOMETRIE_BIAIS_DROITE + ODOMETRIE_NOISE_RATIO * rand_sym());

        SensorData data = {
            .vfiltre1 = v_gauche_mesuree / RAYON_ROUE,
            .vfiltre2 = v_droite_mesuree / RAYON_ROUE,
            .angle = angle_deg
        };
        set_sensor_data(&data);

        // 3️⃣ Simuler une mesure Marvelmind toutes les 2 s
        clock_gettime(CLOCK_MONOTONIC, &t_now);
        double t_now_s = t_now.tv_sec + t_now.tv_nsec / 1e9;
        if (t_now_s - t_last_mm > 1.0/MARVELMIND_UPDATE_FREQ_HZ) {
            MarvelmindPosition mm = {
                .x = (int)(x_real + (rand() % 80 - 40)),
                .y = (int)(y_real + (rand() % 80 - 40)),
                .z = 0,
                .t = t_now,
                .valid = true
            };
            _set_marvelmind_position(mm);
            t_last_mm = t_now_s;
        }

        // 4️⃣ Lecture des positions pour enregistrement
        PositionVoiture pos_fusion = {0};
        get_position(&pos_fusion);
        MarvelmindPosition mm_pos = get_marvelmind_position(false);

        // 5️⃣ Calcul du temps écoulé depuis le début
        double t_s = (t_now.tv_sec + t_now.tv_nsec / 1e9) - t_start;

        // 6️⃣ Écriture dans le CSV
        fprintf(f, "%.3f,%.1f,%.1f,%.1f,%.1f,%.1f,%.1f,%.1f,%.1f,%d\n",
            t_s,
            x_real, y_real,
            pos_fusion.vx, pos_fusion.vy,
            pos_fusion.x, pos_fusion.y,
            mm_pos.x, mm_pos.y,
            mm_pos.valid
        );
        fflush(f);

        my_sleep(SIM_DT);
    }

    fclose(f);
    return NULL;
}

#endif