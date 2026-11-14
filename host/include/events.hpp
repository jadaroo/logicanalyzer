#pragma once

#include <event2/event_struct.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <vector>
#include <iostream>

enum event_name_t : uint8_t{
  START = 0,
  STOP = 1,
  ACK = 2,
  NACK = 3,
  ADDRESS = 4,
  DATA = 5,
  READ = 6,
  WRITE = 7
};

enum class decoding_state{
  IDLE,
  ADDRESS_READ,
  DATA_READ,
  ACK
};

struct event_t {
  event_name_t name;
  size_t sample_idx;
  uint64_t value;
};

inline void print_events(std::vector<event_t> &events, uint64_t sample_rate){
  std::vector<std::string> event_names;
  std::vector<std::string> values;
  std::vector<std::string> timestamps;
  for(auto& event : events){
    if(event.name == START){
      event_names.push_back(" |  START |");
    } else if(event.name == ADDRESS){
      event_names.push_back(" | ADDRESS|");
    } else if(event.name == DATA){
      event_names.push_back(" |   DATA |");
    } else if(event.name == READ){
      event_names.push_back(" |   READ |");
    } else if(event.name == WRITE){
      event_names.push_back(" |  WRITE |");
    } else if(event.name == ACK){
      event_names.push_back(" |   ACK  |");
    } else if(event.name == NACK){
      event_names.push_back(" |   NACK |");
    } else{
      event_names.push_back(" |   STOP |");
    }

    std::string buffer_value(100, '\0');
    snprintf(buffer_value.data(), buffer_value.size(), " |  0x%02X  |", static_cast<unsigned>(event.value));
    values.push_back(buffer_value);
    std::string buffer_time(100, '\0');
    snprintf(buffer_time.data(), buffer_time.size(), " |  %llu  |", static_cast<long long>(event.sample_idx) * static_cast<long long>(1000000000) / static_cast<long long>(sample_rate));
    timestamps.push_back(buffer_time);
  }

  for (auto& name : event_names){
    std::cout << name;
  } 

  std::cout << '\n';

  for(auto& value : values){
    std::cout << value;
  }

  std::cout << '\n';

  for(auto& timestamp : timestamps){
    std::cout << timestamp;
  }

  std::cout << '\n';
}