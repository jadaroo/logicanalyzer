#pragma once

#include "protocols.h"
#include "decoder/decoder.hpp"
#include <stdint.h>
#include <vector>

#define NUM_OF_CHANNELS 8
#define FIRST_PIN 7
#define LAST_PIN (FIRST_PIN + NUM_OF_CHANNELS - 1)

uint8_t pins_needed(protocol_t protocol);
uint8_t count_free_pins(const pins_t &pins);

bool choose_protocol(decoder_config_t &main_config, const pins_t &pins, bool allow_done = false);

uint8_t read_pin(const char *pin_name, const char *protocol_name, pins_t &pins);
void set_up_pins(decoder_config_t &config, pins_t &pins);
void create_decoders(std::vector<decoder_config_t> &decoders);

void set_up_spi(spi_config_t &config);
void choose_freq(decoder_config_t &main_config);
void choose_num_of_samples(capture_config_t &capture);

void convert_to_vcd(std::vector<uint8_t> &data, const capture_config_t &capture);
void send_config_to_pico(int pico, const capture_config_t &capture);

void init(int &pico, std::vector<decoder_config_t> &decoders, capture_config_t &capture);

