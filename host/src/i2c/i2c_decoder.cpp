#include "i2c_decoder.hpp"

#include "events.hpp"
#include <fcntl.h>
#include <stdint.h>
#include <stdbool.h>
#include <vector>

std::vector<event_t> decode(const std::vector<uint8_t> &scl, const std::vector<uint8_t> &sda, uint32_t samples){
  std::vector<event_t> events;

  decoding_state state = decoding_state::IDLE;
  uint8_t address = 0;
  uint8_t adr_idx = 7;
  uint8_t data = 0;
  uint8_t data_idx = 7;
  for(uint32_t i = 1; i < samples; i++){
    if(scl[i] && sda[i - 1] && !sda[i]){
      address = 0;
      adr_idx = 7;
      data = 0;
      data_idx = 7;
      events.push_back({START, i, 0});
      state = decoding_state::ADDRESS_READ;
    } 

    if(scl[i] && !sda[i - 1] && sda[i]){
      address = 0;
      adr_idx = 7;
      data = 0;
      data_idx = 7;
      events.push_back({STOP, i, 1});
    }
    
    switch (state) {
      case decoding_state::IDLE:

        break;

      case decoding_state::ADDRESS_READ:
        if(!scl[i - 1] && scl[i]){
          if(adr_idx <= 7){
            address |= (sda[i] << adr_idx--);
          } 

          if(adr_idx == 255){
            events.push_back({ADDRESS, i, uint8_t(address >> 1u)});
            if(address & (1u << 0)){
              events.push_back({READ, i, 1});
            } else{
              events.push_back({WRITE, i, 0});
            }

            address = 0;
            adr_idx = 7;
            state = decoding_state::ACK;
          }
        }
        
        break;

      case decoding_state::ACK:
        if(!scl[i - 1] && scl[i]){
          if(sda[i]){
            events.push_back({NACK, i, 1});
          } else{
            events.push_back({ACK, i, 0});
          }
          
          state = decoding_state::DATA_READ;
        }

        break;
        
      case decoding_state::DATA_READ:        
        if(sda[i - 1] && !sda[i] && scl[i]){
          events.push_back({START, i, 0});
          data = 0;
          data_idx = 7;
          state = decoding_state::ADDRESS_READ;
        }
        
        if(!scl[i - 1] && scl[i]){
          if(data_idx <= 7){
            data |= (sda[i] << data_idx--);
          } 

          if(data_idx == 255){
            events.push_back({DATA, i, data});

            data = 0;
            data_idx = 7;
            state = decoding_state::ACK;
          }
        }
    }
  }
  
  return events;
}