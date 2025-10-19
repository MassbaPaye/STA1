#include "utils.h"
#include "logger.h"
#include "config.h"
#include <unistd.h> // pour getopt
#include <bits/getopt_core.h>

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
