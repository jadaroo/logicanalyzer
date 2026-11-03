#ifndef SETTINGS_H
#define SETTINGS_H

#include <stdint.h>
#include "protocols.h"

extern protocol_t protocol;
extern uint32_t frequency;
extern uint8_t default_factor;
extern uint32_t baudrate;

void choose_protocol(void);
void choose_freq(void);
void choose_baud(void);

#endif
