#include <stdint.h>
#ifndef I2C_DECODER_H
#define I2C_DECODER_H

#include "events.h"



event_t *decode(const uint8_t *scl, const uint8_t *sda);

#endif