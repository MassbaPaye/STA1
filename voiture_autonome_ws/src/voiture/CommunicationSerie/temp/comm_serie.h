#ifndef COMM_SERIE_H
#define COMM_SERIE_H

#include <stdint.h>

int comm_serie_init(const char* port);
void comm_serie_close(void);

// Envoie une consigne moteur de manière non bloquante
void send_motor_speed(int consigneD, int consigneG);

#endif
