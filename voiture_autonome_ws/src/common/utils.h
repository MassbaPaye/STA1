#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <stdlib.h>

// Déclaration des variables globales pour les ports
extern char *megapi_port;
extern char *marvelmind_port;

// Fonction pour gérer les arguments
void gestion_arguments(int argc, char *argv[]);

#endif // UTILS_H
