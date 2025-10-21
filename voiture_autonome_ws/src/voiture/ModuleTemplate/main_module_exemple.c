#include <stdio.h>
#include <stdbool.h>
#include "utils.h"
#include "voiture_globals.h"
#include <math.h>
#include "logger.h"

#define TAG "module_exemple"

void exemple_module(char* message) {
    INFO(TAG, "%s", message);

}


int affiche_position_actuelle() {
    PositionVoiture t;
    if (get_position(&t) != 0)
    {
        ERR(TAG, "Position de la voiture inconnue");
        return -1;
    }
    INFO(TAG, "\tx: %d", t.x);
    INFO(TAG, "\ty: %d", t.y);
    INFO(TAG, "\tz: %d", t.z);
    INFO(TAG, "\tz: %d", t.z);
    INFO(TAG, "\ttheta: %f", t.theta);
    INFO(TAG, "\tv: %f", sqrt(t.x*t.x + t.y*t.y + t.z*t.z));

    return 0;
}