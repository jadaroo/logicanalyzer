#include "hardware/pio.h"
#include "hardware/i2c.h"
#include "read_program.pio.h"
#include <boards/pico.h>
#include <boards/pico_w.h>
#include <hardware/regs/dreq.h>
#include <hardware/structs/clocks.h>
#include <stdbool.h>
#include <string.h>
#include <hardware/address_mapped.h>
#include <hardware/dma.h>
#include <hardware/gpio.h>
#include <hardware/irq.h>
#include <hardware/regs/intctrl.h>
#include <hardware/structs/dma.h>
#include <hardware/structs/io_bank0.h>
#include <pico/stdio.h>
#include <pico/time.h>
#include <stdint.h>
#include <stdio.h>
#include "hardware/vreg.h"
#include "pico/stdio_usb.h"
#include "protocols.h"
#include "hardware/clocks.h"

#define FIRST_CHANNEL 0
#define BUFFER_SIZE 50000
#define F_SYS 200000000

uint32_t sample_buffer[BUFFER_SIZE];

protocol_t protocol;
uint32_t frequency;
uint32_t baudrate;
uint8_t factor;

/* Read one line from USB serial (without "\r\n"). Blocks until '\n'. */
static void read_line(char *buf, size_t len){
  size_t n = 0;
  for(;;){
    int ch = getchar_timeout_us(100000);
    if(ch == PICO_ERROR_TIMEOUT) continue;
    if(ch == '\r') continue;
    if(ch == '\n'){
      buf[n] = '\0';
      return;
    }
    if(n < len - 1) buf[n++] = (char)ch;
  }
}

/* Wait for a valid "CFG <protocol> <value>" command from the host. */
static void wait_for_config(void){
  char line[CMD_MAX_LEN];
  for(;;){
    read_line(line, sizeof(line));

    unsigned p;
    unsigned long value;
    if(sscanf(line, CMD_CONFIG " %u %lu", &p, &value) != 2){
      printf(RESP_ERR " bad command\n");
      continue;
    }
    if(p > SPI){
      printf(RESP_ERR " bad protocol\n");
      continue;
    }
    if(!config_valid((protocol_t)p, value)){
      printf(RESP_ERR " value out of range\n");
      continue;
    }

    protocol = (protocol_t)p;
    if(protocol == UART){
      baudrate = value;
    } else{
      frequency = value;
    }
    printf(RESP_OK "\n");
    return;
  }
}

static void print_channel(const char *name, uint first_bit){
  printf("%s: ", name);
  for(uint i = 0; i < BUFFER_SIZE; i++){
    for(uint j = first_bit; j < 32; j += 2){
      putchar((sample_buffer[i] & (1u << j)) ? '1' : '0');
    }
  }
  printf("\n");
}

int main(void){
  vreg_set_voltage(VREG_VOLTAGE_1_15);
  set_sys_clock_hz(F_SYS, true);
  
  stdio_init_all();
  printf("%u\n", clock_get_hz(clk_sys));
  while(!stdio_usb_connected());
  
  i2c_init(i2c_default, 100*1000);
  gpio_set_function(PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C);
  gpio_set_function(PICO_DEFAULT_I2C_SDA_PIN, GPIO_FUNC_I2C);

  gpio_pull_up(PICO_DEFAULT_I2C_SCL_PIN);
  gpio_pull_up(PICO_DEFAULT_I2C_SDA_PIN);

  PIO pio;
  uint sm;
  uint offset;

  bool success = pio_claim_free_sm_and_add_program_for_gpio_range(&read_program, &pio, &sm, &offset, 16, 8, true);
  hard_assert(success);

  for(uint8_t i = 16; i < 25; i++){
    gpio_disable_pulls(i);
  }

  
  
  read_program_init(pio, sm, offset, 16);

  dma_channel_claim(FIRST_CHANNEL);

  dma_channel_config c = dma_channel_get_default_config(FIRST_CHANNEL);
  channel_config_set_transfer_data_size(&c, DMA_SIZE_32);
  channel_config_set_read_increment(&c, false);
  channel_config_set_write_increment(&c, true);

  channel_config_set_dreq(&c, pio_get_dreq(pio, sm, false));

  uint8_t data[511];
  memset(&data[0], 0xFF, 511);

  wait_for_config();
  if(protocol == UART || protocol == I2C) factor = 10;
  if(protocol == SPI) factor = 4;

  // TODO: use protocol / frequency / baudrate to set the PIO sample rate
  uint32_t clkdiv;
  if(protocol == UART){
    clkdiv = F_SYS / (baudrate * factor);
  } else{
    clkdiv = F_SYS / (frequency * factor);
  }
  
  pio_sm_set_clkdiv(pio, sm, clkdiv);
  pio_sm_clkdiv_restart(pio, sm);
  pio_sm_set_enabled(pio, sm, false);
  pio_sm_clear_fifos(pio, sm);
  pio_sm_restart(pio, sm);

  dma_channel_configure(FIRST_CHANNEL,
  &c,
  sample_buffer,
  &pio->rxf[sm],
  BUFFER_SIZE,
  true);

  pio_sm_set_enabled(pio, sm, true);
  
  for(;;){
    
    printf(RESP_END "\n");
  }
}
