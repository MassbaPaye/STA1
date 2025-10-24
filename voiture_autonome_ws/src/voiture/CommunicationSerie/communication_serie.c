#include <time.h>
#include <string.h>
#include <unistd.h>  // pour close()
#include "logger.h"
#include "communication_serie.h"
#include "config.h"
#include "utils.h"
#include "messages.h"
#include "voiture_globals.h"
#include "comm_serie_utils.h"

#define READ_SENSOR_FREQ 20
#define TAG "comm_serie"


static int fd_serial = -1;
static pthread_t read_thread;
static pthread_mutex_t serial_write_mutex = PTHREAD_MUTEX_INITIALIZER;

// overrun:0, ref1:100, ref2:80, speed1:59.2637, speed2:51.1728, angle:92.12, vfiltre1:59, vfiltre2:51
static void parse_sensor_line(const char* line, SensorData* data) {
    sscanf(line,
           "overrun:%d, ref1:%f, ref2:%f, speed1:%f, speed2:%f, angle:%f, vfiltre1:%f, vfiltre2:%f",
           &data->overrun,
           &data->ref1, &data->ref2,
           &data->speed1, &data->speed2,
           &data->angle,
           &data->vfiltre1, &data->vfiltre2);
}


// --- thread lecture ---
static void* thread_read(void* arg) {
    (void)arg;
    char line[256];

    while (1) {
        int n = read_line(fd_serial, line, sizeof(line));
        if (n > 0) {
            SensorData local;
            parse_sensor_line(line, &local);
            set_sensor_data(&local);
            /* SensorData global_data;
            get_sensor_data(&global_data, true);
            DBG(TAG, "Données recu : overrun:%d, ref1:%.2f, ref2:%.2f, speed1:%.2f, speed2:%.2f, angle:%.2f, vfiltre1:%.2f, vfiltre2:%.2f\n",
                   global_data.overrun, global_data.ref1, global_data.ref2,
                   global_data.speed1, global_data.speed2, global_data.angle,
                   global_data.vfiltre1, global_data.vfiltre2); 
            fflush(stdout); */
        } else {
            struct timespec ts;
            ts.tv_sec = 0;
            ts.tv_nsec = (long)(1.0 / READ_SENSOR_FREQ * 1e9);
            nanosleep(&ts, NULL);
        }
    }
    return NULL;
}

void* lancer_communication_serie() {
    INFO(TAG, "Thread communication série démarré");
    
    if (strcmp(megapi_port, "stdin") == 0) {
        fd_serial = 0;
    } else {
        fd_serial = open_serial_port(megapi_port, MEGAPI_BAUDRATE);
        ERR(TAG, "Erreur lors de l'ouverture du port %s à %d bauds", megapi_port, MEGAPI_BAUDRATE);
        if (fd_serial < 0) return NULL;
    }

    if (pthread_create(&read_thread, NULL, thread_read, NULL) != 0) {
        perror("pthread_create read_thread");
        return NULL;
    }

    return NULL;
}

void stop_communication_serie() {
    if (fd_serial > 0) close(fd_serial);
    // Ajouter un arret propre de tout
}

void send_motor_speed(float v_left, float v_right) {
    if (fd_serial < 0) return;
    int consigneG = (int) v_left;
    int consigneD = (int) v_right;

    char buffer[64];
    int len = snprintf(buffer, sizeof(buffer),
                       "{\"consigneD\":%d,\"consigneG\":%d}\n",
                       consigneD, consigneG);

    pthread_mutex_lock(&serial_write_mutex);
    write_all(fd_serial, buffer, len);
    pthread_mutex_unlock(&serial_write_mutex);
}