//Serielle Kommunikation
#include "serial.h"
#include <termios.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <sys/ioctl.h>

#ifdef __linux__
#include <linux/termios.h> // für termios2 / BOTHER falls verfügbar
#ifndef TCGETS2
#define TCGETS2 0x802C542A
#endif
#ifndef TCSETS2
#define TCSETS2 0x402C542B
#endif
#endif

static int baud_to_const(int baud){
    switch(baud){
        case 9600: return B9600;
        case 19200: return B19200;
        case 38400: return B38400;
        case 57600: return B57600;
        case 115200: return B115200;
        default: return 0; // 0 signalisiert: kein vordefinierter konstanter Wert
    }
}

int serial_open(const char *device, int baud){
    int fd = open(device, O_RDWR | O_NOCTTY | O_SYNC);
    if(fd < 0){
        perror("open serial");
        return -1;
    }
    struct termios tty;
    if(tcgetattr(fd, &tty) != 0){
        perror("tcgetattr");
        close(fd);
        return -1;
    }

    int bconst = baud_to_const(baud);
    if(bconst != 0){
        cfsetospeed(&tty, bconst);
        cfsetispeed(&tty, bconst);
    } else {
#ifdef __linux__
        // Versuch: benutzerdefinierte Baudrate über termios2 / BOTHER setzen
        struct termios2 tio2;
        if(ioctl(fd, TCGETS2, &tio2) == 0){
            tio2.c_cflag &= ~CBAUD;
#ifdef BOTHER
            tio2.c_cflag |= BOTHER;
#else
            tio2.c_cflag |= BOTHER;
#endif
            tio2.c_ispeed = baud;
            tio2.c_ospeed = baud;
            if(ioctl(fd, TCSETS2, &tio2) != 0){
                // fallback: setze auf 38400
                cfsetospeed(&tty, B38400);
                cfsetispeed(&tty, B38400);
            } else {
                // termios2 erfolgreich gesetzt; wir sollten trotzdem die Standard termios flags setzen unten
            }
        } else {
            // kein termios2 verfügbar: fallback
            cfsetospeed(&tty, B38400);
            cfsetispeed(&tty, B38400);
        }
#else
        // Nicht-Linux: fallback auf 38400
        cfsetospeed(&tty, B38400);
        cfsetispeed(&tty, B38400);
#endif
    }

    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;
    tty.c_iflag &= ~(IGNBRK | ICRNL | IXON | IXOFF | IXANY);
    tty.c_lflag = 0;
    tty.c_oflag = 0;
    tty.c_cc[VMIN]  = 0;
    tty.c_cc[VTIME] = 10; // 1s read timeout
    tty.c_cflag |= (CLOCAL | CREAD);
    tty.c_cflag &= ~(PARENB | PARODD);
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CRTSCTS;
    if(tcsetattr(fd, TCSANOW, &tty) != 0){
        perror("tcsetattr");
        close(fd);
        return -1;
    }
    return fd;
}

ssize_t serial_readline(int fd, char *buf, size_t maxlen){
    if(fd < 0) return 0;
    size_t i = 0;
    char c;
    while(i + 1 < maxlen){
        ssize_t n = read(fd, &c, 1);
        if(n == 0) { // timeout
            if(i==0) return 0;
            break;
        }
        if(n < 0) return -1;
        if(c == '\r') continue;
        if(c == '\n') break;
        buf[i++] = c;
    }
    buf[i] = '\0';
    return (ssize_t)i;
}

void serial_close(int fd){
    if(fd >= 0) close(fd);
}
