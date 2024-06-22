#ifndef BMP180_ESP_IDF_H
#define BMP180_ESP_IDF_H

#include "hal/i2c_types.h"
#include "rom/ets_sys.h"
typedef struct i2c_lowlevel_s
{
    i2c_port_t port;
    int pin_sda;
    int pin_scl;
} i2c_lowlevel_config;

#define bmp_delay_us(x) ets_delay_us(x)

#endif /* BMP180_ESP_IDF_H */