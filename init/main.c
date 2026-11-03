#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "settings.h"
#include "serial.h"

#define DEFAULT_PORT "/dev/ttyACM0"
#define LINE_MAX_LEN 8192

protocol_t protocol;
uint32_t frequency;
uint8_t default_factor;
uint32_t baudrate;

/* Read a line from stdin; exits the program on EOF (e.g. Ctrl+D). */
static void read_input(char *buf, int len){
  if(!fgets(buf, len, stdin)){
    printf("\nNo input, exiting.\n");
    exit(1);
  }
}

void choose_protocol(void){
  while(true){
    char protocol_text[10];
    printf("Choose Protocol(I2C, UART, SPI):\n");
    read_input(protocol_text, sizeof(protocol_text));
    protocol_text[strcspn(protocol_text, "\r\n")] = '\0';
    if(strcmp(protocol_text, "UART") == 0){
      protocol = UART;
      break;
    } else if(strcmp(protocol_text, "I2C") == 0){
      protocol = I2C;
      break;
    } else if(strcmp(protocol_text, "SPI") == 0){
      protocol = SPI;
      break;
    } else{
      printf("Incorrect protocol input. Try again.\n");
    }
  }
}

void choose_freq(void){
  while(true){
    char freq_text[12];
    printf("Enter input signal frequency.\n");
    printf("For SPI 1k - 50 MHz, for I2C 1k - 20 MHz\n");
    read_input(freq_text, sizeof(freq_text));
    char *end;
    errno = 0;
    long freq = strtol(freq_text, &end, 10);
    if(end == &freq_text[0] || (*end != '\n' && *end != '\r' && *end != '\0') || errno == ERANGE){
      printf("Incorrect frequency input. Try again.\n");
      continue;
    }

    if(freq < 0 || !config_valid(protocol, (unsigned long)freq)){
      if(protocol == SPI){
        printf("Incorrect frequency input. Try again. The frequency for SPI should be between 1kHz and 50 MHz\n");
      } else{
        printf("Incorrect frequency input. Try again. The frequency for I2C should be between 1kHz and 20 MHz\n");
      }
      continue;
    }

    frequency = (uint32_t)freq;
    break;
  }
}

void choose_baud(void){
  while(true){
    char baud_text[12];
    printf("Enter UART baudrate (200 - 12.5 M):\n");
    read_input(baud_text, sizeof(baud_text));
    char *end;
    errno = 0;
    long baud = strtol(baud_text, &end, 10);
    if(end == &baud_text[0] || (*end != '\n' && *end != '\r' && *end != '\0') || errno == ERANGE ||
       baud < 0 || !config_valid(UART, (unsigned long)baud)){
      printf("Incorrect baudrate input. Try again. UART baudrate should be between 200 and 12.5 M\n");
      continue;
    }

    baudrate = (uint32_t)baud;
    break;
  }
}

/* Host-side program: asks the user for the capture settings,
 * sends them to the Pico and prints the captured data. */
int main(int argc, char **argv){
  const char *port = argc > 1 ? argv[1] : DEFAULT_PORT;

  choose_protocol();

  if(protocol == UART){
    choose_baud();
  } else{
    choose_freq();
  }

  int fd = serial_open(port);
  if(fd < 0) return 1;

  unsigned long value = (protocol == UART) ? baudrate : frequency;
  char cmd[CMD_MAX_LEN];
  snprintf(cmd, sizeof(cmd), CMD_CONFIG " %d %lu\n", (int)protocol, value);
  printf("Sending: %s", cmd);
  if(serial_write_str(fd, cmd) != 0){
    close(fd);
    return 1;
  }

  static char line[LINE_MAX_LEN];
  if(serial_read_line(fd, line, sizeof(line)) < 0){
    close(fd);
    return 1;
  }
  if(strcmp(line, RESP_OK) != 0){
    fprintf(stderr, "Pico rejected the config: %s\n", line);
    close(fd);
    return 1;
  }

  // Print capture data until the Pico says it is done
  while(serial_read_line(fd, line, sizeof(line)) >= 0){
    if(strcmp(line, RESP_END) == 0){
      close(fd);
      return 0;
    }
    printf("%s\n", line);
  }

  close(fd);
  return 1;
}
