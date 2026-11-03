#include "serial.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

int serial_open(const char *path){
  int fd = open(path, O_RDWR | O_NOCTTY);
  if(fd < 0){
    fprintf(stderr, "Cannot open %s: %s\n", path, strerror(errno));
    return -1;
  }

  struct termios tty;
  if(tcgetattr(fd, &tty) != 0){
    fprintf(stderr, "tcgetattr: %s\n", strerror(errno));
    close(fd);
    return -1;
  }

  cfmakeraw(&tty);
  tty.c_cflag |= CLOCAL | CREAD;
  // USB CDC ignores the baud rate, but it has to be set to something
  cfsetispeed(&tty, B115200);
  cfsetospeed(&tty, B115200);
  // read() returns after at most 5 s without data
  tty.c_cc[VMIN] = 0;
  tty.c_cc[VTIME] = 50;

  if(tcsetattr(fd, TCSANOW, &tty) != 0){
    fprintf(stderr, "tcsetattr: %s\n", strerror(errno));
    close(fd);
    return -1;
  }

  tcflush(fd, TCIOFLUSH);
  return fd;
}

int serial_write_str(int fd, const char *s){
  size_t len = strlen(s);
  while(len > 0){
    ssize_t n = write(fd, s, len);
    if(n < 0){
      if(errno == EINTR) continue;
      fprintf(stderr, "write: %s\n", strerror(errno));
      return -1;
    }
    s += n;
    len -= (size_t)n;
  }
  tcdrain(fd);
  return 0;
}

int serial_read_line(int fd, char *buf, size_t len){
  size_t n = 0;
  for(;;){
    char ch;
    ssize_t r = read(fd, &ch, 1);
    if(r < 0){
      if(errno == EINTR) continue;
      fprintf(stderr, "read: %s\n", strerror(errno));
      return -1;
    }
    if(r == 0){
      fprintf(stderr, "Timeout waiting for the Pico\n");
      return -1;
    }
    if(ch == '\r') continue;
    if(ch == '\n'){
      buf[n] = '\0';
      return (int)n;
    }
    if(n < len - 1) buf[n++] = ch;
  }
}
