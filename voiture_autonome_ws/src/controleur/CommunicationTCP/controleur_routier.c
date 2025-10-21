#include <stdlib.h>
#include "messages.h"
#include "controleur_globals.h"
#include "communication_tcp_controleur.h"

void gerer_demande(int Id){
    Demande *d = malloc(sizeof(Demande));
    Consigne *c = malloc(sizeof(Consigne));
    while (size_demande_queue() > 0){
        dequeue_demande(d);
        if (d->type_operation == 1){
            if ( structure[d->structure_id].etat == 0 ){
                structure[d->structure_id].etat = 1;
                structure[d->structure_id].id = Id;
                c->structure_id = d->structure_id;
                c->autorisation = 0;
                sendConsigne(Id, c);
            } else {
                c->structure_id = d->structure_id;
                c->autorisation = 1;
                sendConsigne(Id, c);
            }
        }
        if (d->type_operation == 0){
            structure[d->structure_id].etat = 0;
            structure[d->structure_id].id = -1;
        }
    }
}
