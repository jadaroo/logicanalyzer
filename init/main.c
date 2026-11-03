#include "stdlib.h"
#include "stdbool.h"
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "protocols.h"

protocol_t protocol;
uint32_t frequency;
uint8_t default_factor;
uint32_t baudrate;

void choose_protocol(void){
  while(true){
    char protocol_text[10];
    printf("Choose Protocol(I2C, UART, SPI):\n");
    if(!fgets(protocol_text, sizeof(protocol_text), stdin)){
      printf("Incorrect protocol input. Try again.\n");
      continue;
    } else{
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
        continue;
      }
    }
  }
}

void choose_freq(void){
  while(true){
    char freq_text[10];
    printf("Enter input signal frequency.\n");
    printf("For SPI 1k - 50 MHz, for I2C 1k - 20 MHz\n");
    if(!fgets(freq_text, 10, stdin)){
      printf("Incorrect frequency input. Try again.\n");
      continue;
    }
    char *end;
    errno = 0;
    long freq = strtol(freq_text, &end, 10);
    if(end == &freq_text[0] || (*end != '\n' && *end != '\r' && *end != '\0') || errno == ERANGE){
      printf("Incorrect frequency input. Try again.\n");
      continue;
    }

    if(protocol == SPI){
      if(freq < 1000 || freq > 50000000){
        printf("Incorrect frequency input. Try again. The frequency for SPI should be between 1kHz and 50 MHz\n");
        continue;
      } else{
        frequency = freq;
        break;
      }
    } else if(protocol == I2C){
      if(freq < 1000 || freq > 20000000){
        printf("Incorrect frequency input. Try again. The frequency for I2C should be between 1kHz and 20 MHz\n");
        continue;
      } else{
        frequency = freq;
        break;
      }
    }
  }
  
}

void choose_baud(void){
  while(true){
    char baud_text[10];
    printf("Enter input baudrate.\n");
    if(!fgets(baud_text, 10, stdin)){
      printf("Incorrect baudrate input. Try again.\n");
      continue;
    }
    char *end;
    errno = 0;
    long baud = strtol(baud_text, &end, 10);
    if(end == &baud_text[0] || (*end != '\n' && *end != '\r' && *end != '\0') || errno == ERANGE){
      printf("Incorrect frequency input. Try again. UART baudrate should be between 200 and 12.5 M\n");
      continue;
    }

    if(baud < 200 || baud > 12500000){
      printf("Incorrect frequency input. Try again. UART baudrate should be between 200 and 12.5 M\n");
      continue;
    }

    baudrate = baud;
    break;
  }
}

int main(void){
  choose_protocol();

  if(protocol == UART){
    choose_baud();
  } else{
    choose_freq();
  }

  

  return 0;
}
