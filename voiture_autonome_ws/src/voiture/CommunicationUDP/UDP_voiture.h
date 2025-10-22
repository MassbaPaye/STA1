#ifndef UDP_VOITURE_H
#define UDP_VOITURE_H

#include "messages.h"

void* initialisation_communication_camera(void* arg);
DonneesDetection* parse_json_to_donnees(const char *json_str);

#endif
