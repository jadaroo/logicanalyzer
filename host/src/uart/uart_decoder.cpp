#include "uart_decoder.hpp"
#include "decoder.hpp"
#include "events.hpp"
#include <ctime>
#include <stdint.h>
#include <vector>

std::vector<event_t> uart_decode(const std::vector<uint8_t> &rx, const uart_config_t &uart_config, const capture_config_t &capture){
  std::vector<event_t> events;

  uint32_t data = 0;
  uint8_t data_idx = 0;

  uint8_t stop = 0;

  decoding_state state = decoding_state::IDLE;
  for(uint32_t i = 1; i < rx.size(); i++){
    switch (state) {
        case decoding_state::IDLE:
          if(rx[i - 1] && !rx[i]){
            events.push_back({START, i, 0});
            state = decoding_state::DATA_READ;
          }

          break;

        case decoding_state::DATA_READ:
          if(data_idx < uart_config.data_bits){
            data |= rx[i] << data_idx++;
          } 

          if(data_idx == uart_config.data_bits){
            events.push_back({DATA, i, data});
            data_idx = 0;
            data = 0;
            if(uart_config.parity == NONE){
              state = decoding_state::STOP;
            } else{
              state = decoding_state::PARITY;
            }
          }

          break;

        case decoding_state::PARITY:
          events.push_back({PARITY, i, rx[i]});

          state = decoding_state::STOP;
          break;

        case decoding_state::STOP:
          if(stop < uart_config.stop_bits){
            events.push_back({STOP, i, rx[i]});
            if(!rx[i]){
              std::cout << "Framing error, stop bit is 0. At " << calculate_timestamp(i, capture) << '\n';
            }
            stop++;
          } 

          if(stop == uart_config.stop_bits){
            stop = 0;
            state = decoding_state::IDLE;
          }

          break;
    }
  }

  return events;
}