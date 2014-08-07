#ifndef _CONNECT_H
#define _CONNECT_H

#include "config.h"

int rs232_config(int fd, int speed, int databits, int stopbits, int parity);
int rs232_flush(int fd);
ssize_t rs232_read(int fd, uint8_t *buf, size_t nbytes);
ssize_t rs232_write(int fd, uint8_t *buf, size_t nbytes);
int rs232_init(char *tty_name, int baudrate);

#endif	/* _CONNECT_H */
