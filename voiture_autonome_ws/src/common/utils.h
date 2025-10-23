#ifndef UTILS_H
#define UTILS_H

#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 199309L
#endif
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "messages.h"

#define PI 3.141592653589793

// Déclaration des variables globales pour les ports
extern char *megapi_port;
extern char *marvelmind_port;

// Fonction pour gérer les arguments
void gestion_arguments(int argc, char *argv[]);

double timespec_diff_s(struct timespec t1, struct timespec t2);
void my_sleep(double t_s);

float distance_from_car(PositionVoiture pv, Point p);

#endif // UTILS_H
