#include "comm_serie.h"
#include "globals.h"
#include "comm_serie_utils.h"

#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

// --- variables internes ---
static int fd_serial = -1;
static pthread_t read_thread;
static pthread_mutex_t serial_write_mutex = PTHREAD_MUTEX_INITIALIZER;

// --- parsing ligne capteurs ---
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

            pthread_mutex_lock(&sensor_mutex);
            sensor_data = local;
            pthread_mutex_unlock(&sensor_mutex);
        } else {
            usleep(1000);
        }
    }
    return NULL;
}

// --- API publique ---

int comm_serie_init(const char* port) {
    if (strcmp(port, "stdin") == 0) {
        fd_serial = 0;
    } else {
        fd_serial = open_serial_port(port, 115200);
        if (fd_serial < 0) return -1;
    }

    if (pthread_create(&read_thread, NULL, thread_read, NULL) != 0) {
        perror("pthread_create read_thread");
        return -1;
    }

    return 0;
}

void comm_serie_close(void) {
    if (fd_serial >= 0 && fd_serial != 0) close(fd_serial);
    // Optionnel: pthread_cancel + pthread_join
}

// --- envoi consigne moteur (bloquant mais simple) ---
void send_motor_speed(int consigneD, int consigneG) {
    if (fd_serial < 0) return;

    char buffer[64];
    int len = snprintf(buffer, sizeof(buffer),
                       "{\"consigneD\":%d,\"consigneG\":%d}\n",
                       consigneD, consigneG);

    pthread_mutex_lock(&serial_write_mutex);
    write_all(fd_serial, buffer, len);
    pthread_mutex_unlock(&serial_write_mutex);
}
