#include <stdlib.h>
#include <stdio.h>
#include "messages.h"
#include "controleur_globals.h"
#include "controleur_routier.h"
#include "communication_tcp_controleur.h"

void gerer_demande(int id){
    Demande *d = malloc(sizeof(Demande));
    Consigne *c = malloc(sizeof(Consigne));
    while (size_demande_queue() > 0){
        dequeue_demande(d);
        if (d->type_operation == 1){
            if ( structure[d->structure_id].etat == 0 ){
                structure[d->structure_id].etat = 1;
                structure[d->structure_id].id = id;
                printf("etat = %d, id = %d \n", structure[d->structure_id].etat, structure[d->structure_id].id)
                c->structure_id = d->structure_id;
                c->autorisation = 0;
                sendMessage(id, MESSAGE_CONSIGNE, c);
            } else {
                c->structure_id = d->structure_id;
                c->autorisation = 1;
                sendMessage(id, MESSAGE_CONSIGNE, c);
            }
        }
        if (d->type_operation == 0){
            structure[d->structure_id].etat = 0;
            structure[d->structure_id].id = -1;
            printf("etat = %d, id = %d \n", structure[d->structure_id].etat, structure[d->structure_id].id)
        }
    }
}
