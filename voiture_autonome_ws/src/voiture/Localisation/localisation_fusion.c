
#include "utils.h"
#include "localisation_fusion.h"
#include "logger.h"
#include "config.h"
#include <math.h>

#define TAG "loc-fusion"

// --- Variables internes ---
static MarvelmindPosition marvelmind_estimee = {0};
static bool marvelmind_init = false;

void estimer_position_apres_dt(float* x, float* y, float* z,
                               float vx, float vy, float vz, double dt) {
    *x += vx * dt;
    *y += vy * dt;
    *z += vz * dt;
}

// Estimation de la positon instantanée à partir des données capteurs
PositionOdom calculer_odometrie(void) {
    SensorData sdata;
    PositionVoiture last_pos;
    struct timespec t_last, t_now;
    PositionOdom odom = {0};

    if (get_sensor_data(&sdata, true) != 0 ||
        get_position(&last_pos, true) != 0) {
        ERR(TAG, "Impossible de récupérer sensor_data ou position_voiture");
        return odom;
    }

    t_last = get_position_last_update();
    clock_gettime(CLOCK_MONOTONIC, &t_now);
    double dt = timespec_diff_s(t_last, t_now);
    if (dt <= 0.0) dt = 0.001;

    float v = RAYON_ROUE * (sdata.vfiltre1 + sdata.vfiltre2) / 2.0;
    // float omega = RAYON_ROUE * (sdata.vfiltre1 - sdata.vfiltre2) / (2.0 * ECARTEMENT_ROUE);

    odom.vx = v * cos(sdata.angle * PI / 180.0);
    odom.vy = v * sin(sdata.angle * PI / 180.0);
    odom.vz = 0;
    odom.theta = sdata.angle;

    odom.x = last_pos.x + odom.vx * dt;
    odom.y = last_pos.y + odom.vy * dt;
    odom.z = last_pos.z + odom.vz * dt;

    return odom;
}

// Estimation de la position instantanée à partir du marvelmind et de la vitesse instantanée de l'odométrie
MarvelmindPosition mettre_a_jour_marvelmind_estimee(const PositionOdom* odom) {
    struct timespec t_now;
    clock_gettime(CLOCK_MONOTONIC, &t_now);

    MarvelmindPosition mm_new_pos = get_marvelmind_position();

    // (1) Initialisation ou fusion si nouvelle mesure valide
    if (mm_new_pos.valid) {
        if (!marvelmind_init) {
            marvelmind_estimee = mm_new_pos;
            marvelmind_init = true;
        } else {
            // Calcul du dt entre la dernière estimation et la nouvelle mesure
            double dt = timespec_diff_s(marvelmind_estimee.t, mm_new_pos.t);
            // if (dt < 0.0) dt = 0.0;

            // Estimer la position Marvelmind au moment de la nouvelle mesure
            float x_est = marvelmind_estimee.x, y_est = marvelmind_estimee.y, z_est = marvelmind_estimee.z;
            estimer_position_apres_dt(&x_est, &y_est, &z_est, odom->vx, odom->vy, odom->vz, dt);

            // Fusion pondérée
            marvelmind_estimee.x = FUSION_ODO_ALPHA * x_est + (1.0 - FUSION_ODO_ALPHA) * mm_new_pos.x;
            marvelmind_estimee.y = FUSION_ODO_ALPHA * y_est + (1.0 - FUSION_ODO_ALPHA) * mm_new_pos.y;
            marvelmind_estimee.z = FUSION_ODO_ALPHA * z_est + (1.0 - FUSION_ODO_ALPHA) * mm_new_pos.z;
            marvelmind_estimee.t = mm_new_pos.t;
            marvelmind_estimee.valid = true;
        }
    }

    // (2) Projection continue à t_now (propagation)
    if (marvelmind_init) {
        double dt_proj = timespec_diff_s(marvelmind_estimee.t, t_now);
        estimer_position_apres_dt(&marvelmind_estimee.x, &marvelmind_estimee.y, &marvelmind_estimee.z,
                                  odom->vx, odom->vy, odom->vz, dt_proj);
        marvelmind_estimee.t = t_now;
    }

    return marvelmind_estimee;
}
