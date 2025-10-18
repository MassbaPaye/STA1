/* main.c */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <signal.h>
#include "localisation.h"
#include "marvelmind.h"
#include "binary_sender.h"

static bool terminateProgram = false;

void CtrlHandler(int signum) {
    terminateProgram = true;
}

int main(int argc, char *argv[]) {
    char *ttyFileName;
    char *remote_ip = "127.0.0.1";
    int remote_port = 6000;

    if (argc >= 2) ttyFileName = argv[1];
    if (argc >= 3) remote_ip = argv[2];
    if (argc >= 4) remote_port = atoi(argv[3]);


    else ttyFileName = DEFAULT_TTY_FILENAME;

    // Gestion du Ctrl-C
    signal(SIGINT, CtrlHandler);
    signal(SIGQUIT, CtrlHandler);

    if (localisation_start(ttyFileName) != 0) {
        return -1;
    }
    struct PositionValue position;
    bool res = get_position(&position);
    printf("res: %b\n", res);
    
    // Etablissement de la connexion serveur
    int sd1 = open_socket();
    connect_to(sd1, remote_ip, remote_port);

    
    while(true) 
    {
        bool res = get_position(&position);
        printf("[%b] Position : %d(x=%d y=%d z=%d angle=%f)\n", res, position.address, position.x, position.y, position.z, position.angle);
        
        send_data(sd1, &position, sizeof(position));
        
        usleep(2000000); // 1 seconde
    }
    // Fermeture de la connexion serveur
    close_socket(sd1);
    localisation_stop();
    return 0;
}
