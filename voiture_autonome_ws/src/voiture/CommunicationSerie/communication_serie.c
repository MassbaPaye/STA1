#include "logger.h"
#include "communication_serie.h"
#include "config.h"
#include "utils.h"

#define TAG "comm_serie"


void* lancer_communication_serie(void* arg) {
    INFO(TAG, "Initialisation de la communication série sur le port %s à %d bauds", megapi_port, SERIAL_BAUDRATE);
    return NULL;
}

int send_motor_consigne(int left_speed, int right_speed) {
    INFO(TAG, "Envoi des consignes moteurs : gauche=%d droite=%d", left_speed, right_speed);
    return -1;
}