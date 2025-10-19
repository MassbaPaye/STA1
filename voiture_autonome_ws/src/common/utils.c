#include "utils.h"
#include "logger.h"
#include "config.h"
#include <unistd.h> // pour getopt
#include <bits/getopt_core.h>
#include <time.h>

// Définition des variables globales
char *megapi_port = DEFAULT_MEGAPI_PORT;
char *marvelmind_port = DEFAULT_MARVELMIND_PORT;

void gestion_arguments(int argc, char *argv[]) {
    int opt;

    // "m:n:" → m et n attendent un argument
    while ((opt = getopt(argc, argv, "p:m:")) != -1) {
        switch (opt) {
            case 'p':  // Megapi port
                megapi_port = optarg;
                INFO("arguments", "Port Megapi défini sur : %s\n", megapi_port);
                break;
            case 'm':  // Marvelmind port
                marvelmind_port = optarg;
                INFO("arguments", "Port Marvelmind défini sur : %s\n", marvelmind_port);
                break;
            default:
                fprintf(stderr, "Usage: %s [-mpp megapi_port] [-mmp marvelmind_port]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }
}


double timespec_diff_s(struct timespec t1, struct timespec t2) {
    return (t2.tv_sec - t1.tv_sec) + (t2.tv_nsec - t1.tv_nsec) / 1e9;
}


void my_sleep(double t_s)
{
    struct timespec sleep_ts;

    sleep_ts.tv_sec = (time_t)t_s;
    sleep_ts.tv_nsec = (long)((t_s - sleep_ts.tv_sec) * 1e9);

    if (sleep_ts.tv_nsec >= 1000000000L) {
        sleep_ts.tv_sec += 1;
        sleep_ts.tv_nsec -= 1000000000L;
    }
    nanosleep(&sleep_ts, NULL);
}