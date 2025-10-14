

#define CHECK_ERROR(val1,val2, msg) if (val1==val2) { perror(msg); exit(EXIT_FAILURE);}

#define MAXOCTETS 80


int main_communication_tcp_controleur();
