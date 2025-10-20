// comm_serie_utils.h
#ifndef COMM_SERIE_UTILS_H
#define COMM_SERIE_UTILS_H

#include <stddef.h>

int open_serial_port(const char* port_name, int baudrate);
int read_line(int fd, char* buffer, size_t max_len);
int write_all(int fd, const char* data, size_t len);

#endif
