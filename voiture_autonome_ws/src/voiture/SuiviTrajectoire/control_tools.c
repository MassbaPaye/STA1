#include "control_tools.h"
#include "messages.h"
#include "math.h"
#include "utils.h"
#include "logger.h"

#define TAG "control-tools"


Point to_absolute(Point pr, PositionVoiture pv) {
    Point pa;
    pa.x = pv.x + cosf(pv.theta) * pr.x - sinf(pv.theta) * pr.y;
    pa.y = pv.y + sinf(pv.theta) * pr.x + cosf(pv.theta) * pr.y;
    pa.theta = pv.theta + pr.theta;
    return pa;
}

Point to_relative(Point pa, PositionVoiture pv) {
    Point pr;
    pr.x =   cosf(pv.theta) * (pa.x - pv.x) + sinf(pv.theta) * (pa.y - pv.y);
    pr.y = - sinf(pv.theta) * (pa.x - pv.x) + cosf(pv.theta) * (pa.y - pv.y);
    pr.theta = pa.theta - pv.theta;
    return pr;
}


Polynome compute_relative_polynome(Point pr) {
    Polynome p;
    float x1 = pr.x;
    float y1 = pr.y;
    float theta1 = pr.theta;
    if (pr.x <= 0)
        ERR(TAG, "Impossible de calculer le polynome (x1=%.1f<=0)", x1);
    if (theta1 <= -PI/2 || theta1 > PI/2)
        ERR(TAG, "Theta invalid (x1=%.1f<=0)", x1);
    
    p.a0 = 0.0;
    p.a1 = 0.0;
    p.a2 = 3*y1/(x1*x1) - tanf(theta1)/x1;
    p.a3 = tanf(theta1)/(x1*x1) - 2*y1/(x1*x1*x1);
    return p;
}

Point eval_polynome_relative(Polynome poly, float x) {
    if (poly.a0 == 0 && poly.a1 == 0)
    {
        ERR(TAG, "Polynome invalide a0=%.1f, a1=%.1f sont non nuls");
    }
    Point p;
    p.x = x;
    p.y = poly.a0 + poly.a1*x + poly.a2*x*x + poly.a3*x*x*x;
    p.z = 0;
    p.theta = atanf(poly.a1 + 2*poly.a2*x + 3*poly.a3*x*x);
    return p;
}

Point eval_polynome_absolute(Polynome poly, float x, PositionVoiture pv) {
    return to_absolute(eval_polynome_relative(poly, x), pv);
}



int find_closest_point(PositionVoiture pv, Trajectoire traj) {
    if (traj.nb_points <= 0) {
        ERR(TAG, "Trajectoire vide");
    }
    int closest_point_id = 0;
    float best_dist = distance_from_car(pv, traj.points[0]);
    float new_dist;
    for (int i = 0; i < traj.nb_points; i++)
    {
        new_dist = distance_from_car(pv, traj.points[i]);
        if (new_dist < best_dist)
        {
            best_dist = new_dist;
            closest_point_id = i;
        }
    }
    return closest_point_id;
}

int is_point_overtaken(PositionVoiture voiture, Point p) {
    float scalar_prod = (voiture.x - p.x) * cosf(p.theta) + (voiture.y - p.y) * sinf(p.theta);
    return scalar_prod >= 0;
}

