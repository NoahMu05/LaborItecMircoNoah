#ifndef SERIAL_H
#define SERIAL_H
#include <sys/types.h>
#include <unistd.h>

int serial_open(const char *device, int baud);
ssize_t serial_readline(int fd, char *buf, size_t maxlen);
void serial_close(int fd);

#endif