#ifndef COMMUNICATION_SERIE_H
#define COMMUNICATION_SERIE_H

void* lancer_communication_serie();
void stop_communication_serie();

void envoyer_consigne_moteur(int left_speed, int right_speed);

#endif // COMMUNICATION_SERIE_H