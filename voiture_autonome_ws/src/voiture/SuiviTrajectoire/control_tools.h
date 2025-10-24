#include "messages.h"

Point to_absolute(Point pr, PositionVoiture pv);
Point to_relative(Point pa, PositionVoiture pv);

typedef struct {
    float a0;
    float a1;
    float a2;
    float a3;
} Polynome;


Polynome compute_relative_polynome(Point pr);
Point eval_polynome_relative(Polynome poly, float x);
Point eval_polynome_absolute(Polynome poly, float x, PositionVoiture pv);

int find_closest_point(PositionVoiture pv, Trajectoire traj);

int is_point_overtaken(PositionVoiture voiture, Point p);

Point compute_projection_using_l1(PositionVoiture voiture, Point p1_abs, Polynome poly);

void compute_errors(PositionVoiture voiture, Point p_proj_abs);