#include "spi_decoder.hpp"
#include "events.hpp"
#include "main_config.hpp"
#include <cstdint>
#include <stdint.h>
#include <vector>

void spi_decode(const std::vector<uint8_t> &cs, const std::vector<uint8_t> &scl, 
              const std::vector<uint8_t> &mosi, const std::vector<uint8_t> &miso, 
              const spi_config_t &config, uint32_t samples,
              std::vector<event_t> &mosi_events, std::vector<event_t> &miso_events, 
              config_t &main_config){
          
  uint32_t mosi_word = 0;
  uint32_t mosi_idx = config.word_size - 1;

  uint32_t miso_word = 0;
  uint32_t miso_idx = config.word_size - 1;

  for(uint32_t i = 1; i < samples; i++){
    if(cs[i - 1] && !cs[i]){
      mosi_events.push_back({START, i, 0});
      miso_events.push_back({START, i, 0});
      while(!cs[i]){        
        if(config.mode == MODE_0 ||config.mode == MODE_3){
          if(!scl[i - 1] && scl[i]){
            if(miso_idx <= config.word_size - 1){
              miso_word |= miso[i] << miso_idx--;
              mosi_word |= mosi[i] << mosi_idx--;
            }

            if(miso_idx == UINT32_MAX || mosi_idx == UINT32_MAX){
              miso_events.push_back({DATA, i, miso_word});
              mosi_events.push_back({DATA, i, mosi_word});

              mosi_word = 0;
              miso_word = 0;
              miso_idx = config.word_size - 1;
              mosi_idx = config.word_size - 1;
            }
          }
        } else if(config.mode == MODE_1 || config.mode == MODE_2){
          if(scl[i - 1] && !scl[i]){
            if(miso_idx <= config.word_size - 1){
              miso_word |= miso[i] << miso_idx--;
              mosi_word |= mosi[i] << mosi_idx--;
            }

            if(miso_idx == UINT32_MAX || mosi_idx == UINT32_MAX){
              miso_events.push_back({DATA, i, miso_word});
              mosi_events.push_back({DATA, i, mosi_word});

              mosi_word = 0;
              miso_word = 0;
              miso_idx = config.word_size - 1;
              mosi_idx = config.word_size - 1;
            }
          } 
        }
        i++;
      } 
      
      if((miso_word != 0 && miso_idx != config.word_size - 1) || (mosi_word != 0 && miso_idx != config.word_size - 1)){
        std::cout << "No complete word on timestamp " << calculate_timestamp(i, main_config) << ", only " << config.word_size - 1 - miso_idx << " bits captured.\n"; 
      }
    }
  } 

  if(mosi_events.empty() && miso_events.empty()){
    std::cout << "No data was captured, start position wasn't captured, or samples are empty.\n";
  }
  
}