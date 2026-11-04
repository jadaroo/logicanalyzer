#include "protocols.h"
#include <termios.h>
#include <stdbool.h>
#include <math.h>
#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

protocol_t protocol;
uint32_t frequency;
uint32_t num_samples;
uint8_t default_factor;

#define PROTOCOL_INPUT_SIZE 6
#define FREQUENCY_INPUT_SIZE 10
#define MIN_FREQ 1000
#define SPI_MAX_FREQ 50000000
#define I2C_MAX_FREQ 3400000
#define UART_MAX_FREQ 12500000
#define UART_MIN_FREQ 305
#define MAX_SAMPLES 200000
#define SAMPLES_INPUT_SIZE 8

void choose_protocol(){
  while(true){
    char buffer[PROTOCOL_INPUT_SIZE];
    printf("Enter the needed protocol(UART, I2C, SPI):\n");
    if(!fgets(buffer, sizeof(buffer), stdin)){
      printf("No protocol was read.\n");
      exit(EXIT_FAILURE);
    }

    size_t end_of_input_idx = strcspn(buffer, "\n");
    if(end_of_input_idx == sizeof(buffer) - 1){
      printf("Invalid input, try again.\n");
      while(getchar() != '\n');
      continue;
    }

    buffer[end_of_input_idx] = '\0';
    if(strcasecmp("SPI", buffer) == 0){
      protocol = SPI;
      default_factor = 4;
    } else if(strcasecmp("I2C", buffer) == 0){
      protocol = I2C;
      default_factor = 10;
    } else if(strcasecmp("UART", buffer) == 0){
      protocol = UART;
      default_factor = 10;
    } else{
      printf("Unknown protocol\n");
      continue;
    }

    break;
  }
}

void choose_freq(){
  while(true){
    if(protocol == SPI){
      printf("Enter the signal frequency(1 k to 50 M), without Hz:\n");
    } else if(protocol == I2C){
      printf("Enter the signal frequency(1 k to 3.4 M), without Hz:\n");
    } else{
      printf("Enter the baudrate(200 to 12.5 Mbaud):, without Hz\n");
    }

    char buffer[FREQUENCY_INPUT_SIZE];
    if(!fgets(buffer, sizeof(buffer), stdin)){
      exit(EXIT_FAILURE);
    }

    size_t end_of_input_idx = strcspn(buffer, "\n");
    if(end_of_input_idx == sizeof(buffer) - 1){
      printf("Invalid input, try again.\n");
      while(getchar() != '\n');
      continue;
    }

    buffer[end_of_input_idx] = '\0';
    char *end_ptr;
    double frequency_temp = strtod(buffer, &end_ptr);
    if(protocol == SPI){
      if(!(frequency_temp >= MIN_FREQ && frequency_temp <= SPI_MAX_FREQ)){
        printf("Too large frequency for SPI.\n");
        continue;
      }
    } else if(protocol == I2C){
      if(!(frequency_temp >= MIN_FREQ && frequency_temp <= I2C_MAX_FREQ)){
        printf("Too large frequency for I2C.\n");
        continue;
      }
    } else{
      if(!(frequency_temp >= UART_MIN_FREQ && frequency_temp <= UART_MAX_FREQ)){
        printf("Too large baudrate for UART.\n");
        continue;
      }
    }
    
    if(end_ptr == buffer){
      printf("No number was read.\n");
      continue;
    }
    while(isspace((unsigned char)*end_ptr)) end_ptr++;

    uint32_t multiplier;
    if(*end_ptr == '\0') multiplier = 1;
    else if(*end_ptr == 'k' || *end_ptr == 'K'){
      multiplier = (uint32_t)1000;
      end_ptr++;
    } 
    else if(*end_ptr == 'M') {
      multiplier = (uint32_t)1000000;
      end_ptr++;
    }
    else{
      printf("Wrong input, frequency should be only without multiplier or in kilo or mega Hz, like this: 1k, 5M\n");
      continue;
    }

    while(isspace((unsigned char)*end_ptr)) end_ptr++;
    if(*(end_ptr + 1) != '\0'){
      printf("Wrong input, frequency should be only without multiplier or in kilo or mega Hz, like this: 1k, 5M\n");
      continue;
    }
    
    frequency_temp *= multiplier;
    frequency_temp = lround(frequency_temp);

    frequency = frequency_temp;

    break;
  }
}

void choose_num_of_samples(){
  while(true){
    printf("Enter the number of needed samples(1-200000):\n");
    char buffer[SAMPLES_INPUT_SIZE];
    if(!fgets(buffer, sizeof(buffer), stdin)){
      printf("No number was read.\n");
      exit(EXIT_FAILURE);
    }

    size_t end_of_input_idx = strcspn(buffer, "\n");
    if(end_of_input_idx == sizeof(buffer) - 1){
      printf("Invalid input, try again.\n");
      while(getchar() != '\n');
      continue;
    }

    buffer[end_of_input_idx] = '\0';
    
    char *end_ptr;
    long samples = strtol(buffer, &end_ptr, 10);
    if(end_ptr == buffer){
      printf("No number was read.\n");
      continue;
    }
    while(isspace((unsigned char)*end_ptr)) end_ptr++;
    
    if(!(samples > 0 && samples < MAX_SAMPLES + 1)){
      printf("Wrong input, number of samples should be only integer, from 1-200000\n");
      continue;
    }

    if(*end_ptr != '\0'){
      printf("Wrong input, number of samples should be only integer, from 1-200000\n");
      continue;
    }

    num_samples = (uint32_t)samples;

    break;
  }
}

int main(void){
    struct termios settings;

    int pico = open("/dev/pico", O_RDWR | O_NOCTTY);
    if(pico == -1){
      perror("ERROR open /dev/pico\n");
      exit(EXIT_FAILURE);
    }

    if(tcgetattr(pico, &settings) == -1){
      perror("ERROR open /dev/pico\n");
      exit(EXIT_FAILURE);
    }
    cfmakeraw(&settings);
    settings.c_cc[VMIN] = 0;
    settings.c_cc[VTIME] = 10;
    if(tcsetattr(pico, TCSANOW, &settings) == -1){
      perror("ERROR open /dev/pico\n");
      exit(EXIT_FAILURE);
    }
    if(tcflush(pico, TCIFLUSH) == -1){
      perror("ERROR open /dev/pico\n");
      exit(EXIT_FAILURE);
    }
    
    choose_protocol();
    choose_freq();
    choose_num_of_samples();

    char final_buffer[100];
    int len = snprintf(final_buffer, sizeof(final_buffer), "FREQ %u", frequency * default_factor);
    if(len == -1){
      perror("Error in sending commands.\n");
      exit(EXIT_FAILURE);
    }
    
    ssize_t written = write(pico, final_buffer, len);
}
