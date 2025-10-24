#ifndef COMMUNICATION_SERIE_H
#define COMMUNICATION_SERIE_H

void* lancer_communication_serie();
void stop_communication_serie();

void set_motor_speed(float v_left, float v_right);

#endif // COMMUNICATION_SERIE_H