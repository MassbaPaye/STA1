#include "utils.h"
#include "logger.h"
#include "config.h"
#include <time.h>
#include <string.h>
#include "messages.h"
#include "math.h"

#ifdef _WIN32
#include <windows.h>
#endif

// Définition des variables globales
char *megapi_port = DEFAULT_MEGAPI_PORT;
char *marvelmind_port = DEFAULT_MARVELMIND_PORT;

static void print_usage(const char *prog_name) {
    fprintf(stderr,
            "Usage: %s [-p <megapi_port>] [-m <marvelmind_port>]\n"
            "Options équivalentes : --megapi=<port>, --marvelmind=<port>\n",
            prog_name);
}

void gestion_arguments(int argc, char *argv[]) {
    for (int i = 1; i < argc; ++i) {
        const char *arg = argv[i];

        if ((strcmp(arg, "-p") == 0 || strcmp(arg, "--megapi-port") == 0)) {
            if (i + 1 >= argc) {
                print_usage(argv[0]);
                exit(EXIT_FAILURE);
            }
            megapi_port = argv[++i];
            INFO("arguments", "Port Megapi défini sur : %s\n", megapi_port);
        } else if (strncmp(arg, "--megapi=", 9) == 0) {
            megapi_port = (char *)(arg + 9);
            INFO("arguments", "Port Megapi défini sur : %s\n", megapi_port);
        } else if ((strcmp(arg, "-m") == 0 || strcmp(arg, "--marvelmind-port") == 0)) {
            if (i + 1 >= argc) {
                print_usage(argv[0]);
                exit(EXIT_FAILURE);
            }
            marvelmind_port = argv[++i];
            INFO("arguments", "Port Marvelmind défini sur : %s\n", marvelmind_port);
        } else if (strncmp(arg, "--marvelmind=", 13) == 0) {
            marvelmind_port = (char *)(arg + 13);
            INFO("arguments", "Port Marvelmind défini sur : %s\n", marvelmind_port);
        } else if (strcmp(arg, "-h") == 0 || strcmp(arg, "--help") == 0) {
            print_usage(argv[0]);
            exit(EXIT_SUCCESS);
        } else {
            fprintf(stderr, "Option inconnue: %s\n", arg);
            print_usage(argv[0]);
            exit(EXIT_FAILURE);
        }
    }
}


double timespec_diff_s(struct timespec t1, struct timespec t2) {
    return (t2.tv_sec - t1.tv_sec) + (t2.tv_nsec - t1.tv_nsec) / 1e9;
}


void my_sleep(double t_s)
{
#ifdef _WIN32
    if (t_s <= 0.0) {
        return;
    }
    DWORD duration_ms = (DWORD)(t_s * 1000.0);
    Sleep(duration_ms > 0 ? duration_ms : 1);
#else
    struct timespec sleep_ts;

    sleep_ts.tv_sec = (time_t)t_s;
    sleep_ts.tv_nsec = (long)((t_s - sleep_ts.tv_sec) * 1e9);

    if (sleep_ts.tv_nsec >= 1000000000L) {
        sleep_ts.tv_sec += 1;
        sleep_ts.tv_nsec -= 1000000000L;
    }
    nanosleep(&sleep_ts, NULL);
#endif
}

float distance_from_car(PositionVoiture pv, Point p) {
    return sqrtf((pv.x-p.x)*(pv.x-p.x) - (pv.y-p.y)*(pv.y-p.y));
}