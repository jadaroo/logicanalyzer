#pragma once

#include "decoder.hpp"
#include "events.hpp"
#include <stdint.h>
#include <vector>

std::vector<event_t> uart_decode(const std::vector<uint8_t> &rx, const uart_config_t &uart_config, const capture_config_t &capture);