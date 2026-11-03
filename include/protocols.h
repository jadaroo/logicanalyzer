#ifndef PROTOCOLS_H
#define PROTOCOLS_H

#include <stdbool.h>
#include <stdint.h>

typedef enum{
  I2C = 0,
  UART = 1,
  SPI = 2
} protocol_t;

/* Limits shared by the host program and the firmware */
#define SPI_FREQ_MIN   1000UL
#define SPI_FREQ_MAX   50000000UL
#define I2C_FREQ_MIN   1000UL
#define I2C_FREQ_MAX   20000000UL
#define UART_BAUD_MIN  200UL
#define UART_BAUD_MAX  12500000UL

/*
 * Host <-> Pico command protocol (text lines over USB serial):
 *   host -> pico:  "CFG <protocol> <value>\n"   value = frequency (Hz) or UART baudrate
 *   pico -> host:  "OK\n" or "ERR <reason>\n"
 *   pico -> host:  capture data lines, then "END\n"
 */
#define CMD_CONFIG      "CFG"
#define RESP_OK         "OK"
#define RESP_ERR        "ERR"
#define RESP_END        "END"
#define CMD_MAX_LEN     64

static inline bool config_valid(protocol_t p, unsigned long value){
  switch(p){
    case I2C:  return value >= I2C_FREQ_MIN  && value <= I2C_FREQ_MAX;
    case SPI:  return value >= SPI_FREQ_MIN  && value <= SPI_FREQ_MAX;
    case UART: return value >= UART_BAUD_MIN && value <= UART_BAUD_MAX;
  }
  return false;
}

#endif
