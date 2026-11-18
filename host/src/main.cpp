#include "decoder.hpp"
#include "events.hpp"
#include "i2c_decoder.hpp"
#include "main_config.hpp"
#include "protocols.h"
#include "spi_decoder.hpp"
#include <cstdlib>
#include <iostream>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <vector>

int main(void){
    int pico;
    std::vector<decoder_config_t> decoders;
    capture_config_t capture;
    init(pico, decoders, capture);

    std::vector<uint8_t> data(capture.num_samples);

    uint32_t received = 0;
    while(received < capture.num_samples){
      ssize_t r = read(pico, data.data() + received, capture.num_samples - received);
      if(r == -1){
        perror("Reading error\n");
        exit(EXIT_FAILURE);
      } else if(r == 0){
        continue;
      }

      received += r;
    }

    convert_to_vcd(data, capture);
    
    std::vector<std::vector<uint8_t>> channels(8);
    
    for(uint32_t i = 0; i < NUM_OF_CHANNELS; i++){
      for(uint32_t j = 0; j < capture.num_samples; j++){
        if(data[j] & (1u << i)){
          channels[i].push_back(1);
        } else{
          channels[i].push_back(0);
        }
      }
    }

    for(const decoder_config_t &decoder : decoders){
      if(decoder.protocol == SPI){
        const spi_pins_t &pins = decoder.spi_pins;
        std::vector<event_t> spi_mosi;
        std::vector<event_t> spi_miso;
        spi_decode(channels[pins.cs], channels[pins.scl], channels[pins.mosi], channels[pins.miso],
                   decoder.spi_config, capture.num_samples, spi_mosi, spi_miso, capture);
        std::cout << "MOSI DATA:\n";
        print_events(spi_mosi, capture.sample_rate);
        std::cout << "MISO DATA:\n";
        print_events(spi_miso, capture.sample_rate);
      } else if(decoder.protocol == I2C){
        std::vector<event_t> i2c_events = i2c_decode(channels[decoder.i2c_pins.scl], channels[decoder.i2c_pins.sda], capture.num_samples);
        std::cout << "I2C DATA:\n";
        print_events(i2c_events, capture.sample_rate);
      } else if(decoder.protocol == UART){
        std::cout << "UART decoder is not implemented yet.\n";
      }
    }
}
