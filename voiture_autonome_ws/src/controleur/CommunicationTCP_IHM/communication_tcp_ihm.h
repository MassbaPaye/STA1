#ifndef COMMUNICATION_TCP_IHM_H
#define COMMUNICATION_TCP_IHM_H

#include <stddef.h>
#include "messages.h"

extern int sock;

typedef struct {
    int voiture_id;
    Itineraire iti;
} MsgIti;

typedef struct {
    int voiture_id;
    PositionVoiture pos;
} MsgPos;

int sendMessage_ihm( int id,  const PositionVoiture* pos);

void* initialisation_communication_ihm(void* arg);

#endif
