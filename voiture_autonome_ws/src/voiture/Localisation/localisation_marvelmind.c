/* localisation.c */

#include <stdio.h>
#include <stdbool.h>
#include <fcntl.h>
#include <unistd.h>
#include <semaphore.h>
#include <time.h>
#include "localisation.h"
#include "marvelmind.h"

// Variables internes
static sem_t *sem = NULL;
static struct MarvelmindHedge *hedge = NULL;


static void semCallback(void) {
    if (sem != NULL) {
        sem_post(sem);
    }
}

int localisation_start(const char *ttyFileName) {
    hedge = createMarvelmindHedge();
    if (hedge == NULL) {
        puts("Error: Unable to create MarvelmindHedge");
        return -1;
    }

    hedge->ttyFileName=ttyFileName;
    hedge->verbose=true; // show errors and warnings
    hedge->anyInputPacketCallback= semCallback;
    startMarvelmindHedge (hedge);

    sem = sem_open(DATA_INPUT_SEMAPHORE, O_CREAT, 0777, 0);
    if (sem == SEM_FAILED) {
        perror("sem_open");
        return -1;
    }

    printf("Module localisation démarré\n");
    return 0;
}

void localisation_stop(void) {
    if (hedge != NULL) {
        stopMarvelmindHedge(hedge);
        destroyMarvelmindHedge(hedge);
        hedge = NULL;
    }

    if (sem != NULL) {
        sem_close(sem);
        sem_unlink(DATA_INPUT_SEMAPHORE);
        sem = NULL;
    }

    printf("Localisation arrêtée\n");
}


bool get_position(struct PositionValue * position) 
{
    if (hedge == NULL)
    {
        perror("Marvelmind not initialised");
        return -1;
    }
    return getPositionFromMarvelmindHedge(hedge, position);

}


int wait_for_position(int timeout_sec) {
    if (sem == NULL) return -1;

    struct timespec ts;
    if (clock_gettime(CLOCK_REALTIME, &ts) == -1) {
        perror("clock_gettime");
        return -1;
    }

    ts.tv_sec += timeout_sec;

    int ret = sem_timedwait(sem, &ts);
    if (ret == -1) {
        return -1;
    }
    return 0;
}
