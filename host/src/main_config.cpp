#include "main_config.hpp"
#include "decoder.hpp"
#include "protocols.h"
#include <cctype>
#include <cmath>
#include <fcntl.h>
#include <ctime>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>

#define PROTOCOL_INPUT_SIZE 6
#define FREQUENCY_INPUT_SIZE 10
#define MIN_FREQ 1000
#define SPI_MAX_FREQ 50000000
#define I2C_MAX_FREQ 3400000
#define UART_MAX_FREQ 12500000
#define UART_MIN_FREQ 305
#define MAX_SAMPLES 200000
#define SAMPLES_INPUT_SIZE 8

uint8_t pins_needed(protocol_t protocol){
  if(protocol == I2C){
    return 2;
  } else if(protocol == SPI){
    return 4;
  } else{
    return 1;
  }
  
  return 0;
}

uint8_t count_free_pins(const pins_t &pins){
  uint8_t free_pins = 0;
  for(const pin_t &pin : pins.pins){
    if(!pin.is_taken) free_pins++;
  }
  return free_pins;
}

bool choose_protocol(decoder_config_t &main_config, const pins_t &pins, bool allow_done){
  while(true){
    char buffer[PROTOCOL_INPUT_SIZE];
    if(allow_done){
      printf("Enter the needed protocol(UART, I2C, SPI), or DONE to finish:\n");
    } else{
      printf("Enter the needed protocol(UART, I2C, SPI):\n");
    }
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
    if(allow_done && strcasecmp("DONE", buffer) == 0){
      return false;
    }

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

    uint8_t free_pins = count_free_pins(pins);
    if(pins_needed(main_config.protocol) > free_pins){
      printf("Not enough free pins: %s needs %u, only %u left.\n", buffer,
             pins_needed(main_config.protocol), free_pins);
      continue;
    }

    return true;
  }
}

uint8_t read_pin(const char *pin_name, const char *protocol_name, pins_t &pins){
  while(true){
    std::cout << "Choose the " << pin_name << " pin for the " << protocol_name << " (" << FIRST_PIN << " - " << LAST_PIN << "):\n";

    int pin;
    if(!(std::cin >> pin)){
      if(std::cin.eof()){
        std::cout << "No pin was read.\n";
        exit(EXIT_FAILURE);
      }
      std::cout << "Invalid input\n";
      std::cin.clear();
      std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
      continue;
    }
    
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    if(!(pin >= FIRST_PIN && pin <= LAST_PIN)){
      std::cout << "The pin range is " << FIRST_PIN << " - " << LAST_PIN << ".\n";
      continue;
    }

    uint8_t channel = static_cast<uint8_t>(pin - FIRST_PIN);
    if(pins.pins[channel].is_taken){
      std::cout << "The pin is already being used by other decoder.\n";
      continue;
    }

    pins.pins[channel].is_taken = true;
    return channel;
  }
}

void set_up_pins(decoder_config_t &config, pins_t &pins){
  if(config.protocol == I2C){
    config.i2c_pins.scl = read_pin("SCL", "I2C", pins);
    config.i2c_pins.sda = read_pin("SDA", "I2C", pins);
  } else if(config.protocol == SPI){
    config.spi_pins.scl  = read_pin("SCK", "SPI", pins);
    config.spi_pins.cs   = read_pin("CS", "SPI", pins);
    config.spi_pins.mosi = read_pin("MOSI", "SPI", pins);
    config.spi_pins.miso = read_pin("MISO", "SPI", pins);
  } else if(config.protocol == UART){
    config.uart_pins.rx = read_pin("RX", "UART", pins);
  }
}

void create_decoders(std::vector<decoder_config_t> &decoders){
  pins_t pins;
  while(count_free_pins(pins) != 0){
    decoder_config_t decoder;

    if(!choose_protocol(decoder, pins, !decoders.empty())){
      break;
    }
    set_up_pins(decoder, pins);
    choose_freq(decoder);
    if(decoder.protocol == SPI){
      set_up_spi(decoder.spi_config);
    } else if(decoder.protocol == UART){
      set_up_uart(decoder.uart_config);
    }
    decoders.push_back(decoder);
  }
}

void set_up_spi(spi_config_t &config){
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

    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    config.word_size = word_size;
    break;
  }
}

void set_up_uart(uart_config_t &config){
  while(true){
    std::cout << "Enter UART databits number:\n";
    uint32_t databits;
    if(!(std::cin >> databits)){
      std::cout << "Invalid input\n";
      std::cin.clear();
      std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
      continue;
    }

    if(!(databits >= 5 && databits <= 9)){
      std::cout << "Supported size of uart databits are 5 to 9.\n";
      continue;
    }

    config.data_bits = databits;
    break;
  }
  
  while(true){
    std::cout << "Enter UART parity(0 - NONE, 1 - ODD, 2 - EVEN, 3 - MARK, 4 - SPACE):\n";
    int input;
    if(!(std::cin >> input)){
      std::cout << "Invalid input.\n";
      std::cin.clear();
      std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
      continue;
    }

    if(!(input >= 0 && input <= 4)){
      std::cout << "Please enter 0 - NONE, 1 - ODD, 2 - EVEN, 3 - MARK, 4 - SPACE.\n";
      std::cin.clear();
      std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
      continue;
    }

    config.parity = static_cast<parity_t>(input);
    break;
  }
  
  while(true){
    std::cout << "Enter the number of STOP bits, 1 or 2:\n";
    int input;
    if(!(std::cin >> input)){
      std::cout << "Invalid input.\n";
      std::cin.clear();
      std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
      continue;
    }

    if(!(input == 1 || input == 2)){
      std::cout << "Please enter 1 or 2.\n";
      std::cin.clear();
      std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
      continue;
    }

    config.stop_bits = static_cast<uint8_t>(input);
    break;
  }

  while(true){
    std::cout << "Enter the MSB or LSB first, 1(MSB) or 0(LSB):\n";
    int input;
    if(!(std::cin >> input)){
      std::cout << "Invalid input.\n";
      std::cin.clear();
      std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
      continue;
    }

    if(!(input == 1 || input == 0)){
      std::cout << "Please enter 1 or 0.\n";
      std::cin.clear();
      std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
      continue;
    }

    config.msb = static_cast<uint8_t>(input);
    break;
  }

  while(true){
    std::cout << "Enter polarity, 1(high) or 0(low):\n";
    int input;
    if(!(std::cin >> input)){
      std::cout << "Invalid input.\n";
      std::cin.clear();
      std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
      continue;
    }

    if(!(input == 1 || input == 0)){
      std::cout << "Please enter 1 or 0.\n";
      std::cin.clear();
      std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
      continue;
    }

    config.polarity = static_cast<uint8_t>(input);
    break;
  }
}


void choose_freq(decoder_config_t &main_config){
  while(true){
    if(main_config.protocol == SPI){
      printf("Enter the signal frequency(1 k to 50 M), without Hz:\n");
    } else if(main_config.protocol == I2C){
      printf("Enter the signal frequency(1 k to 3.4 M), without Hz:\n");
    } else{
      printf("Enter the baudrate(200 to 12.5 Mbaud):, without baud\n");
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

void choose_num_of_samples(capture_config_t &capture){
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

    capture.num_samples = (uint32_t)samples;

    break;
  }
}

void convert_to_vcd(std::vector<uint8_t> &data, const capture_config_t &capture){
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
  for(size_t i = 1; i < capture.num_samples; i++){
    uint64_t timestamp = static_cast<long long>(i) * static_cast<long long>(1000000000) / capture.sample_rate;
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

  uint64_t closing_timestamp = static_cast<long long>(1000) * static_cast<long long>(1000000000) / capture.sample_rate;
  out << '#' << closing_timestamp << '\n';
  fclose(vcd);
}

void send_config_to_pico(int pico, const capture_config_t &capture){
  struct termios settings;
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

  printf("Sample rate: %u, num of samples: %u\n", capture.sample_rate, capture.num_samples);
  
  char final_buffer[100];
  int len = snprintf(final_buffer, sizeof(final_buffer), "FREQ %u SAMPLES %u\n", capture.sample_rate, capture.num_samples);
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

void init(int &pico, std::vector<decoder_config_t> &decoders, capture_config_t &capture){
  pico = open("/dev/pico", O_RDWR | O_NOCTTY);
  if(pico == -1){
    perror("ERROR open /dev/pico");
    exit(EXIT_FAILURE);
  }

  create_decoders(decoders);
  choose_num_of_samples(capture);

  capture.sample_rate = 0;
  for(const decoder_config_t &decoder : decoders){
    uint32_t rate = decoder.frequency * decoder.default_factor;
    if(rate > capture.sample_rate){
      capture.sample_rate = rate;
    }
  }

  send_config_to_pico(pico, capture);
}
