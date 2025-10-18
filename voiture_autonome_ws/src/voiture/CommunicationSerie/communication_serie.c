#include "logger.h"
#include "communication_serie.h"
#include "config.h"

#define TAG "comm_serie"

char* serial_port = SERIAL_PORT_DEFAULT;

void* lancer_communication_serie(void* arg) {
    char* port = (char*) arg;
    if (port) {
        serial_port = port;
    }
    INFO(TAG, "Initialisation de la communication série sur le port %s à %d bauds", serial_port, SERIAL_BAUDRATE);
    return NULL;
}

int send_motor_consigne(int left_speed, int right_speed) {
    INFO(TAG, "Envoi des consignes moteurs : gauche=%d droite=%d", left_speed, right_speed);
    return -1;
}