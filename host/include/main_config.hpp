#pragma once

#include "protocols.h"
#include <cctype>
#include <cmath>
#include <ctime>
#include <fstream>
#include <ios>
#include <iostream>
#include <limits>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <vector>

struct config_t {
  protocol_t protocol;
  uint32_t frequency;
  uint32_t num_samples;
  uint8_t default_factor;
};

enum spi_mode_t : uint8_t {
  MODE_0 = 0,
  MODE_1 = 1,
  MODE_2 = 2,
  MODE_3 = 3
};

struct spi_config_t {
  spi_mode_t mode;
  uint32_t msb;
  uint32_t word_size;
};

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

inline void choose_protocol(config_t &main_config){
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
      main_config.protocol = SPI;
      main_config.default_factor = 4;
    } else if(strcasecmp("I2C", buffer) == 0){
      main_config.protocol = I2C;
      main_config.default_factor = 10;
    } else if(strcasecmp("UART", buffer) == 0){
      main_config.protocol = UART;
      main_config.default_factor = 10;
    } else{
      printf("Unknown protocol\n");
      continue;
    }

    break;
  }
}

inline void set_up_spi(spi_config_t &config){
  uint32_t main_cpol;
  uint32_t main_cpha;
  while(true){
    std::cout << "Enter SPI CPOL(0 or 1) value:\n";
    uint32_t cpol;
    if(!(std::cin >> cpol)){
      std::cout << "Invalid input\n";
      std::cin.clear();
      std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
      continue;
    }

    if(!(cpol == 0 || cpol == 1)){
      std::cout << "CPOL value should be 0 or 1.\n";
      continue;
    }

    main_cpol = cpol;
    break;
  }

  while(true){
    std::cout << "Enter SPI CPHA(0 or 1) value:\n";
    uint32_t cpha;
    if(!(std::cin >> cpha)){
      std::cout << "Invalid input.\n";
      std::cin.clear();
      std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
      continue;
    }

    if(!(cpha == 0 || cpha == 1)){
      std::cout << "CPHA value should be 1 or 0\n";
      std::cin.clear();
      std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
      continue;
    }

    main_cpha = cpha;
    break;
  }

  if(!main_cpol && !main_cpha) config.mode = MODE_0;
  if(!main_cpol && main_cpha) config.mode = MODE_1;
  if(main_cpol && !main_cpha) config.mode = MODE_2;
  if(main_cpol && main_cpha) config.mode = MODE_3;
  
  while(true){
    std::cout << "Enter SPI MSB(1) or LSB(0)\n";

    uint32_t msb;
    if(!(std::cin >> msb)){
      std::cin.clear();
      std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
      std::cout << "Invalid input.\n";
      continue;
    }

    if(!(msb == 0 || msb == 1)){
      std::cout << "Please enter 1 for MSB and 0 for LSB\n";
      std::cin.clear();
      std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
      continue;
    }

    config.msb = msb;
    break;
  }

  while(true){
    std::cout << "Enter SPI word size, 4 to 32 bits\n";

    uint32_t word_size;
    if(!(std::cin >> word_size)){
      std::cin.clear();
      std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
      std::cout << "Invalid input.\n";
      continue;
    }

    if(!(word_size >= 4 && word_size <= 32)){
      std::cout << "Please enter size between 4 and 32\n";
      std::cin.clear();
      std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
      continue;
    }

    config.word_size = word_size;
    break;
  }
}

inline void choose_freq(config_t &main_config){
  while(true){
    if(main_config.protocol == SPI){
      printf("Enter the signal frequency(1 k to 50 M), without Hz:\n");
    } else if(main_config.protocol == I2C){
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
    while(std::isspace((unsigned char)*end_ptr)) end_ptr++;

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

    if(main_config.protocol == SPI){
      if(!(frequency_temp >= MIN_FREQ && frequency_temp <= SPI_MAX_FREQ)){
        printf("Too large or small frequency for SPI.\n");
        continue;
      }
    } else if(main_config.protocol == I2C){
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

    main_config.frequency = frequency_temp;

    break;
  }
}

inline void choose_num_of_samples(config_t &main_config){
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

    main_config.num_samples = (uint32_t)samples;

    break;
  }
}

inline void convert_to_vcd(std::vector<uint8_t> &data, config_t &main_config){
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
  for(size_t i = 1; i < main_config.num_samples; i++){
    uint64_t timestamp = static_cast<long long>(i) * static_cast<long long>(1000000000) / (main_config.frequency * main_config.default_factor);
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

  uint64_t closing_timestamp = static_cast<long long>(1000) * static_cast<long long>(1000000000) / (main_config.frequency * main_config.default_factor);
  out << '#' << closing_timestamp << '\n';
  fclose(vcd);
}

inline void send_config_to_pico(int &pico, config_t &main_config, struct termios &settings){
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

  printf("FREQ: %u, num of samples: %u\n", main_config.frequency, main_config.num_samples);
  
  char final_buffer[100];
  int len = snprintf(final_buffer, sizeof(final_buffer), "FREQ %u SAMPLES %u\n", main_config.frequency * main_config.default_factor, main_config.num_samples);
  if(len == -1){
    perror("Error in sending commands.\n");
    exit(EXIT_FAILURE);
  }
  
  ssize_t written = write(pico, final_buffer, len);
  if(written == -1){
    perror("Writing error\n");
    exit(EXIT_FAILURE);
  }
}