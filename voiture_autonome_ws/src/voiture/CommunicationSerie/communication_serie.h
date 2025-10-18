
extern char* serial_port;

void* lancer_communication_serie(void* arg);

int send_motor_consigne(int left_speed, int right_speed);