#pragma once

#include <event2/event_struct.h>
#include <stdint.h>
#include <stdlib.h>

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
  uint8_t value;
};