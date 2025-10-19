#include <stdio.h>
#include "main_localisation.h"
#include "logger.h"
#include "marvelmind_manager.h"
#include "utils.h"
#include "config.h"
#include "time.h"

#define TAG "localisation"



void* start_localisation()
{
    if (USE_MARVELMIND) {
        if (start_marvelmind(marvelmind_port) != 0) {
            ERR(TAG, "Impossible de démarrer le module Marvelmind.\n");
            return NULL;
        }

    }

    while (1) {
        if (wait_for_position(5) == 0) {
            MarvelmindPosition pos = get_marvelmind_position();
            DBG(TAG, "Position marvelmind : x=%d y=%d z=%d (t=%d)", pos.x, pos.y, pos.z, (int) (pos.t.tv_sec*1000 + pos.t.tv_nsec/10));
        } else {
            DBG(TAG, "Aucune position reçue depuis 5 secondes");
        }
    }

    

    INFO(TAG, "Fin de Localisation");
    return NULL;
}