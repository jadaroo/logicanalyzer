#include "events.hpp"
#include "i2c_decoder.hpp"
#include "main_config.hpp"
#include "protocols.h"
#include "spi_decoder.hpp"
#include <cstdlib>
#include <termios.h>
#include <stdbool.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <vector>

config_t main_config;
spi_config_t spi_config;

int main(void){
    struct termios settings;

    int pico = open("/dev/pico", O_RDWR | O_NOCTTY);
    if(pico == -1){
      perror("ERROR open /dev/pico\n");
      exit(EXIT_FAILURE);
    }

    choose_protocol(main_config);
    choose_freq(main_config);
    choose_num_of_samples(main_config);

    if(main_config.protocol == SPI){
      set_up_spi(spi_config);
    }

    send_config_to_pico(pico, main_config, settings);

    std::vector<uint8_t> data(main_config.num_samples);

    uint32_t received = 0;
    while(received < main_config.num_samples){
      ssize_t r = read(pico, data.data() + received, main_config.num_samples - received);
      if(r == -1){
        perror("Reading error\n");
        exit(EXIT_FAILURE);
      } else if(r == 0){
        continue;
      }

      received += r;
    }

    convert_to_vcd(data, main_config);
    
    std::vector<std::vector<uint8_t>> channels(8);
    
    for(uint32_t i = 0; i < NUM_OF_CHANNELS; i++){
      for(uint32_t j = 0; j < main_config.num_samples; j++){
        if(data[j] & (1u << i)){
          channels[i].push_back(1);
        } else{
          channels[i].push_back(0);
        }
      }
    }

    std::vector<event_t> i2c_events = i2c_decode(channels[0], channels[1], channels[0].size());
    std::vector<event_t> spi_miso;
    std::vector<event_t> spi_mosi;

    const std::vector<uint8_t> cs   = {1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1};
    const std::vector<uint8_t> sck  = {1,1,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,1,1};
    const std::vector<uint8_t> mosi = {0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,0,0};
    const std::vector<uint8_t> miso = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0};
    
    spi_decode(cs, sck, mosi, miso, spi_config, sck.size(), spi_mosi, spi_miso, main_config);

    if(main_config.protocol == SPI){
      std::cout << "MOSI DATA:\n";
      print_events(spi_mosi, main_config.frequency * main_config.default_factor);
      std::cout << "MISO DATA:\n";
      print_events(spi_miso, main_config.frequency * main_config.default_factor);
    } else if(main_config.protocol == I2C){
      std::cout << "I2C DATA:\n";
      print_events(i2c_events, main_config.frequency * main_config.default_factor);
    } 
}
