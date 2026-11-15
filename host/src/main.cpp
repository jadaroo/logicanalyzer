#include "events.hpp"
#include "i2c_decoder.hpp"
#include "main_config.hpp"
#include "protocols.h"
#include <cstddef>
#include <cstdlib>
#include <fstream>
#include <string>
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

    std::vector<event_t> i2c_events = decode(channels[0], channels[1], channels[0].size());
    
    print_events(i2c_events, main_config.frequency * main_config.default_factor);
}
