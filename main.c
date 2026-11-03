#include "hardware/pio.h"
#include "hardware/i2c.h"
#include "read_program.pio.h"
#include <boards/pico.h>
#include <boards/pico_w.h>
#include <hardware/regs/dreq.h>
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
#include "pico/stdio_usb.h"

#define FIRST_CHANNEL 0
#define BUFFER_SIZE 128

uint32_t sample_buffer[BUFFER_SIZE];

int main(void){
  stdio_init_all();

  while(!stdio_usb_connected());

  i2c_init(i2c_default, 100*1000);
  gpio_set_function(PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C);
  gpio_set_function(PICO_DEFAULT_I2C_SDA_PIN, GPIO_FUNC_I2C);

  gpio_pull_up(PICO_DEFAULT_I2C_SCL_PIN);
  gpio_pull_up(PICO_DEFAULT_I2C_SDA_PIN);

  sleep_ms(3000);

  // TODO: receive protocol / frequency / baudrate from the host program
  // (init/main.c) over USB serial.

  PIO pio;
  uint sm;
  uint offset;

  bool success = pio_claim_free_sm_and_add_program_for_gpio_range(&read_program, &pio, &sm, &offset, 16, 2, true);
  hard_assert(success);

  gpio_disable_pulls(16);
  gpio_disable_pulls(17);

  read_program_init(pio, sm, offset, 16);

  dma_channel_claim(FIRST_CHANNEL);

  dma_channel_config c = dma_channel_get_default_config(FIRST_CHANNEL);
  channel_config_set_transfer_data_size(&c, DMA_SIZE_32);
  channel_config_set_read_increment(&c, false);
  channel_config_set_write_increment(&c, true);

  channel_config_set_dreq(&c, pio_get_dreq(pio, sm, false));

  dma_channel_configure(FIRST_CHANNEL,
  &c,
  sample_buffer,
  &pio->rxf[sm],
  BUFFER_SIZE,
  false);

  uint8_t data[511];
  memset(&data[0], 0xFF, 511);
  dma_channel_start(FIRST_CHANNEL);

  pio_sm_set_enabled(pio, sm, true);
  i2c_write_blocking(i2c_default, 0x7F, data, 511, false);
  dma_channel_wait_for_finish_blocking(FIRST_CHANNEL);

  for(;;){
    printf("SDA: ");
    for(uint8_t i = 0; i < 128; i++){
      for(uint8_t j = 0; j < 32; j += 2){
        if(sample_buffer[i] & (1u << j)){
          printf("%u", 1);
        } else{
          printf("%u", 0);
        }
      }
    }
    printf("\n");
    printf("SCL: ");
    for(uint8_t i = 0; i < 128; i++){
      for(uint8_t j = 1; j < 32; j += 2){
        if(sample_buffer[i] & (1u << j)){
          printf("%u", 1);
        } else{
          printf("%u", 0);
        }
      }
    }

    sleep_ms(60000);
  }
}
