// comm_serie_utils.c
#include "comm_serie_utils.h"
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>

int open_serial_port(const char* port_name, int baudrate) {
    int fd = open(port_name, O_RDWR | O_NOCTTY | O_SYNC);
    if (fd < 0) { perror("open_serial_port"); return -1; }

    struct termios tty;
    memset(&tty, 0, sizeof(tty));
    if (tcgetattr(fd, &tty) != 0) { perror("tcgetattr"); close(fd); return -1; }

    cfsetospeed(&tty, baudrate == 115200 ? B115200 : B9600);
    cfsetispeed(&tty, baudrate == 115200 ? B115200 : B9600);

    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;
    tty.c_iflag &= ~IGNBRK;
    tty.c_lflag = 0;
    tty.c_oflag = 0;
    tty.c_cc[VMIN] = 0;
    tty.c_cc[VTIME] = 5;
    tty.c_iflag &= ~(IXON | IXOFF | IXANY);
    tty.c_cflag |= (CLOCAL | CREAD);
    tty.c_cflag &= ~(PARENB | PARODD);
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CRTSCTS;

    if (tcsetattr(fd, TCSANOW, &tty) != 0) { perror("tcsetattr"); close(fd); return -1; }

    return fd;
}

int read_line(int fd, char* buffer, size_t max_len) {
    size_t pos = 0;
    char c;
    while (pos < max_len - 1) {
        int n = read(fd, &c, 1);
        if (n <= 0) break;
        if (c == '\n') { buffer[pos] = '\0'; return pos; }
        buffer[pos++] = c;
    }
    buffer[pos] = '\0';
    return pos;
}

int write_all(int fd, const char* data, size_t len) {
    size_t total = 0;
    while (total < len) {
        ssize_t n = write(fd, data + total, len - total);
        if (n < 0) return -1;
        total += n;
    }
    return 0;
}
