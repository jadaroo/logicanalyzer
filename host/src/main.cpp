#include "events.hpp"
#include "i2c_decoder.hpp"
#include "protocols.h"
#include <cstddef>
#include <cstdlib>
#include <fstream>
#include <string>
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
#include <time.h>
#include <vector>
#include <iostream>

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
#define NUM_OF_CHANNELS 8

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
      int c = ' ';
      while(c != '\n' && c != EOF){
        c = getchar();
      }
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
    if(*end_ptr != '\0'){
      printf("Wrong input, frequency should be only without multiplier or in kilo or mega Hz, like this: 1k, 5M\n");
      continue;
    }
    
    frequency_temp *= multiplier;
    frequency_temp = lround(frequency_temp);

    if(protocol == SPI){
      if(!(frequency_temp >= MIN_FREQ && frequency_temp <= SPI_MAX_FREQ)){
        printf("Too large or small frequency for SPI.\n");
        continue;
      }
    } else if(protocol == I2C){
      if(!(frequency_temp >= MIN_FREQ && frequency_temp <= I2C_MAX_FREQ)){
        printf("Too large or small frequency for I2C.\n");
        continue;
      }
    } else{
      if(!(frequency_temp >= UART_MIN_FREQ && frequency_temp <= UART_MAX_FREQ)){
        printf("Too large or small baudrate for UART.\n");
        continue;
      }
    }

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

void convert_to_vcd(std::vector<uint8_t> &data){
  char name[100];
  time_t t = time(NULL);
  strftime(name, sizeof(name), "../../captures/capture_%Y%m%d_%H%M%S.vcd", localtime(&t));

  FILE *vcd = fopen(name, "wb");
  std::ofstream out(name);

  out << "$timescale 1 ns $end\n" << "$scope module logic $end\n";

  char channels[8];
  
  for(uint8_t i = 0; i < 8; i++){
    channels[i] = 'a' + i;
    out << "$var wire 1 " << static_cast<char>('a' + i) << " D" << static_cast<char>('0' + i) << " $end\n";  
  }
  out << "$upscope $end\n" << "$enddefinitions $end\n" << "#0\n" << "$dumpvars\n";

  uint8_t prev_sample = data[0];

  for(uint8_t i = 0; i < 8; i++){
    if(prev_sample & (1u << i)){
      out << '1' << channels[i] << '\n';
    } else{
      out << '0' << channels[i] << '\n';
    }
  }
  out << "$end\n";
  
  uint8_t sample;
  for(size_t i = 1; i < num_samples; i++){
    uint64_t timestamp = static_cast<long long>(i) * static_cast<long long>(1000000000) / (frequency * default_factor);
    sample = data[i];
    if(prev_sample != sample){
      out << '#' << timestamp << '\n';
      for(uint8_t i = 0; i < 8; i++){
       if(sample & (1u << i)){
         out << '1' << channels[i] << '\n';
       } else{
         out << '0' << channels[i] << '\n';
       }
      }
    }

    prev_sample = sample;
  }

  uint64_t closing_timestamp = static_cast<long long>(1000) * static_cast<long long>(1000000000) / (frequency * default_factor);
  out << '#' << closing_timestamp << '\n';
  fclose(vcd);
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

    printf("FREQ: %u, num of samples: %u\n", frequency, num_samples);
    
    char final_buffer[100];
    int len = snprintf(final_buffer, sizeof(final_buffer), "FREQ %u SAMPLES %u\n", frequency * default_factor, num_samples);
    if(len == -1){
      perror("Error in sending commands.\n");
      exit(EXIT_FAILURE);
    }
    
    ssize_t written = write(pico, final_buffer, len);
    if(written == -1){
      perror("Writing error\n");
      exit(EXIT_FAILURE);
    }

    std::vector<uint8_t> data(num_samples);

    uint32_t received = 0;
    while(received < num_samples){
      ssize_t r = read(pico, data.data() + received, num_samples - received);
      if(r == -1){
        perror("Reading error\n");
        exit(EXIT_FAILURE);
      } else if(r == 0){
        continue;
      }

      received += r;
    }

    convert_to_vcd(data);
    
    std::vector<std::vector<uint8_t>> channels(8);
    
    for(uint32_t i = 0; i < NUM_OF_CHANNELS; i++){
      for(uint32_t j = 0; j < num_samples; j++){
        if(data[j] & (1u << i)){
          channels[i].push_back(1);
        } else{
          channels[i].push_back(0);
        }
      }
    }

    const std::vector<uint8_t> t3_scl = {
      1,1,1,  // idle
      1,1,  // START
      0,0,1,1,0,0,1,1,0,0,1,1,0,0,1,1,0,0,1,1,0,0,1,1,0,0,1,1,0,0,1,1,  // byte 0x78 (addr 0x3C + W)
      0,0,1,1,  // ACK
      0,0,1,1,0,0,1,1,0,0,1,1,0,0,1,1,0,0,1,1,0,0,1,1,0,0,1,1,0,0,1,1,  // byte 0x10
      0,0,1,1,  // ACK
      0,0,1,1,1,1,  // repeated START
      0,0,1,1,0,0,1,1,0,0,1,1,0,0,1,1,0,0,1,1,0,0,1,1,0,0,1,1,0,0,1,1,  // byte 0x79 (addr 0x3C + R)
      0,0,1,1,  // ACK
      0,0,1,1,0,0,1,1,0,0,1,1,0,0,1,1,0,0,1,1,0,0,1,1,0,0,1,1,0,0,1,1,  // byte 0x5A
      0,0,1,1,  // NACK
      0,0,1,1,1,1,1,  // STOP
    };
    const std::vector<uint8_t> t3_sda = {
      1,1,1,  // idle
      0,0,  // START
      0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,  // byte 0x78 (addr 0x3C + W)
      0,0,0,0,  // ACK
      0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  // byte 0x10
      0,0,0,0,  // ACK
      0,1,1,1,0,0,  // repeated START
      0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,1,1,1,  // byte 0x79 (addr 0x3C + R)
      1,0,0,0,  // ACK
      0,0,0,0,0,1,1,1,1,0,0,0,0,1,1,1,1,1,1,1,1,0,0,0,0,1,1,1,1,0,0,0,  // byte 0x5A
      0,1,1,1,  // NACK
      1,0,0,0,1,1,1,  // STOP
    };
    // expected: START, ADDRESS 0x3C, WRITE, ACK, DATA 0x10, ACK, START, ADDRESS 0x3C, READ, ACK, DATA 0x5A, NACK, STOP
    
    std::vector<event_t> i2c_events = decode(t3_scl, t3_sda, t3_scl.size());
    
    print_events(i2c_events, frequency * default_factor);
}
