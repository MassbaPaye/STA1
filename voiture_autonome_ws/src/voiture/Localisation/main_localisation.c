#include <stdio.h>
#include "logger.h"

#define TAG "localisation"

int start_localisation(char* message)
{
    INFO(TAG, "%s", message);
    return 0;
}