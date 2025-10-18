#include <stdio.h>
#include "logger.h"

#define TAG "localisation"

void* start_localisation(void* message)
{

    INFO(TAG, "%s", (char*)message);
    return NULL;
}