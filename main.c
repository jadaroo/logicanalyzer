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
#define SAMPLES_PER_WORD 4
#define BUFFER_SIZE (MAX_SAMPLES / SAMPLES_PER_WORD)
#define F_SYS 200000000

uint32_t sample_buffer[BUFFER_SIZE];

uint32_t sample_rate;
uint32_t num_samples;

/* Read one line from USB serial (without "\r\n"). Blocks until '\n'. */
static void read_line(char *buf, size_t len){
  
}

/* Wait for a valid "CFG <sample rate> <number of samples>" command from the host. */
static void wait_for_config(void){
  
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

  for(;;){
    wait_for_config();
    // Round up to whole words: the PIO only pushes full 32-bit words
    uint32_t num_words = (num_samples + SAMPLES_PER_WORD - 1) / SAMPLES_PER_WORD;

    uint32_t clkdiv = F_SYS / sample_rate;
    
    pio_sm_set_clkdiv(pio, sm, clkdiv);
    pio_sm_clkdiv_restart(pio, sm);
    pio_sm_clear_fifos(pio, sm);
    pio_sm_restart(pio, sm);
    pio_sm_set_enabled(pio, sm, true);
    
    dma_channel_configure(FIRST_CHANNEL,
    &c,
    sample_buffer,
    &pio->rxf[sm],
    num_words,
    true);

    dma_channel_wait_for_finish_blocking(FIRST_CHANNEL);

    pio_sm_set_enabled(pio, sm, false);
    
    printf(RESP_END "\n");
  }
}
