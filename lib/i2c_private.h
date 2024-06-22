/* \copyright 2024 Zorxx Software. All rights reserved.
 * \license This file is released under the MIT License. See the LICENSE file for details.
 * \brief i2c lowlevel interface 
 */
#ifndef _I2C_PRIVATE_H
#define _I2C_PRIVATE_H

#include <stdbool.h>
#include <stdint.h>

#if defined(__linux__)
   #include "bmp180/i2c_linux.h"
#elif defined(ESP_PLATFORM)
   #include "bmp180/i2c_esp.h"
#else
   #error "Supported OS type not detected"
#endif

typedef void *i2c_lowlevel_context;

i2c_lowlevel_context i2c_ll_init(uint8_t i2c_address, uint32_t i2c_speed, uint32_t i2c_timeout_ms,
                                 i2c_lowlevel_config *config);
bool i2c_ll_deinit(i2c_lowlevel_context ctx);
bool i2c_ll_write(i2c_lowlevel_context ctx, uint8_t reg, uint8_t *data, uint8_t length);
bool i2c_ll_read(i2c_lowlevel_context ctx, uint8_t reg, uint8_t *data, uint8_t length);

#endif /* _I2C_PRIVATE_H */
