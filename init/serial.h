#ifndef SERIAL_H
#define SERIAL_H

#include <stddef.h>

/* Open the Pico's USB serial port in raw mode. Returns fd or -1. */
int serial_open(const char *path);

/* Write a whole string. Returns 0 on success, -1 on error. */
int serial_write_str(int fd, const char *s);

/* Read one line (without "\r\n"). Returns length, or -1 on error/timeout. */
int serial_read_line(int fd, char *buf, size_t len);

#endif
