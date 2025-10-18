/* marvelmind.h */

#ifndef LOCALISATION_H
#define LOCALISATION_H

#include <stdbool.h>
#include "marvelmind.h"

typedef struct {
    int x;
    int y;
    int z;
    timespec t;
} MarvelmindPosition;

int localisation_start(const char *ttyFileName);
void localisation_stop(void);

bool get_position(struct PositionValue * position);
int wait_for_position(int timeout_sec);

#endif
