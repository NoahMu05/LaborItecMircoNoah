#ifndef SERIAL_H
#define SERIAL_H

#include "itec.h"

int configure_com_port(const char *port, int serial_port);
int configure_serial(int fd);
float convert_to_sensor_val(const char *line);
ssize_t read_sim(int serial_fd,  char* chunk, size_t chunk_len);
int connect_to_sensor(char * tty_path);
float read_sensor_value( int serial_fd, char *chunk, char *line_buffer, size_t *line_len);

#endif // SERIAL_H
