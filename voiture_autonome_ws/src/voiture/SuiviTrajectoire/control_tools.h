#include "messages.h"

Point to_absolute(Point pr, PositionVoiture pv);
Point to_relative(Point pa, PositionVoiture pv);

typedef struct {
    float a0;
    float a1;
    float a2;
    float a3;
} Polynome;



Polynome compute_polynome(Point p1, Point p2);
