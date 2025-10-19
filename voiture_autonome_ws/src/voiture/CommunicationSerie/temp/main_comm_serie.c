#include "comm_serie.h"
#include "globals.h"
#include <pthread.h>
#include <unistd.h>
#include <stdio.h>

int main() {
    init_globals();
    
    comm_serie_init("stdin");
    
    // Exemple : envoi périodique de consignes
    while (1) {
        //send_motor_speed(120, 120);
        //send_motor_speed(10, 10);

        pthread_mutex_lock(&sensor_mutex);
        printf("[RECU] Speed1=%.1f | Speed2=%.1f | Angle=%.2f\n",
               sensor_data.speed1, sensor_data.speed2, sensor_data.angle);
        pthread_mutex_unlock(&sensor_mutex);

        sleep(1);
    }
    comm_serie_close();
    cleanup_globals();
    return 0;
}
