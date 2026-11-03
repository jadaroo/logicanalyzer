#ifndef PROTOCOLS_H
#define PROTOCOLS_H

#include <stdint.h>

typedef enum{
  I2C = 0,
  UART = 1,
  SPI = 2
} protocol_t;

extern protocol_t protocol;
extern uint32_t frequency;
extern uint8_t default_factor;
extern uint32_t baudrate;

void choose_protocol(void);
void choose_freq(void);
void choose_baud(void);

#endif
