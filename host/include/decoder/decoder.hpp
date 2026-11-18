#pragma once

#include "protocols.h"
#include <cstdint>
#include <vector>

struct pin_t {
  bool is_taken = false;
};

struct pins_t {
  pin_t pins[8];
};

enum class decoding_state{
  IDLE,
  ADDRESS_READ,
  DATA_READ,
  ACK
};

struct i2c_pins_t {
  uint8_t scl, sda;
};

struct spi_pins_t {
  uint8_t mosi, miso, scl, cs;
};

struct uart_pins_t {
  uint8_t rx;
};

enum parity_t : uint8_t{
  NONE = 0,
  ODD = 1,
  EVEN = 2,
  MARK = 3,
  SPACE = 4
};

struct uart_config_t {
  uart_pins_t pins;
  uint8_t data_bits;
  parity_t parity;
  uint8_t stop_bits;
  uint8_t msb;
  uint8_t polarity;
  uint32_t baudrate;
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


struct capture_config_t {
  uint32_t sample_rate;
  uint32_t num_samples;
};

struct decoder_config_t {
  protocol_t protocol;
  uint32_t frequency;
  uint8_t default_factor;
  i2c_pins_t i2c_pins;
  spi_pins_t spi_pins;
  uart_pins_t uart_pins;
  spi_config_t spi_config;
  uart_config_t uart_config;
};