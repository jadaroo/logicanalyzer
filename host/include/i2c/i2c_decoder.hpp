#pragma once

#include <stdint.h>
#include <vector>
#include "events.hpp"

std::vector<event_t> decode(const std::vector<uint8_t> &scl, const std::vector<uint8_t> &sda, uint32_t samples);