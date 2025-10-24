#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>

int main(void) {
    const char *portname = "/dev/ttyUSB0";    // adapte selon ton port
    int fd = open(portname, O_RDWR | O_NOCTTY | O_SYNC);
    if (fd < 0) {
        perror("Erreur d'ouverture du port série");
        return 1;
    }

    // --- Configuration du port série ---
    struct termios tty;
    memset(&tty, 0, sizeof tty);
    if (tcgetattr(fd, &tty) != 0) {
        perror("Erreur tcgetattr");
        close(fd);
        return 1;
    }

    cfsetospeed(&tty, B115200);   // même baud que Serial.begin(115200)
    cfsetispeed(&tty, B115200);

    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;     // 8 bits
    tty.c_iflag &= ~IGNBRK;
    tty.c_lflag = 0;                                // mode raw
    tty.c_oflag = 0;
    tty.c_cc[VMIN]  = 0;
    tty.c_cc[VTIME] = 5;                            // timeout lecture

    tty.c_iflag &= ~(IXON | IXOFF | IXANY);         // pas de contrôle de flux
    tty.c_cflag |= (CLOCAL | CREAD);                // activer réception
    tty.c_cflag &= ~(PARENB | PARODD);              // pas de parité
    tty.c_cflag &= ~CSTOPB;                         // 1 stop bit
    tty.c_cflag &= ~CRTSCTS;                        // pas de contrôle RTS/CTS

    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        perror("Erreur tcsetattr");
        close(fd);
        return 1;
    }

    // --- Laisser l'Arduino redémarrer après ouverture du port ---
    sleep(2);

    // --- 1. Envoyer la première commande ---
    const char *cmd1 = "{\"consigneG\":50,\"consigneD\":50}\r\n";
    write(fd, cmd1, strlen(cmd1));
    tcdrain(fd); // attendre la fin d'envoi
    printf("Commande envoyée : %s\n", cmd1);

    // --- 2. Attendre 5 secondes ---
    sleep(5);

    // --- 3. Envoyer la commande avec valeurs nulles ---
    const char *cmd2 = "{\"consigneG\":0,\"consigneD\":0}\r\n";
    write(fd, cmd2, strlen(cmd2));
    tcdrain(fd);
    printf("Commande envoyée : %s\n", cmd2);

    // --- 4. Fermer le port ---
    close(fd);
    printf("Port série fermé.\n");
    return 0;
}