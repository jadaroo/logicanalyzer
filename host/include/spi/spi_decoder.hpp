#pragma once

#include <stdint.h>
#include <vector>
#include "events.hpp"
#include "decoder/decoder.hpp"

void spi_decode(const std::vector<uint8_t> &cs, const std::vector<uint8_t> &scl, 
              const std::vector<uint8_t> &mosi, const std::vector<uint8_t> &miso, 
              const spi_config_t &config, uint32_t samples,
              std::vector<event_t> &mosi_events, std::vector<event_t> &miso_events,
              const capture_config_t &capture);