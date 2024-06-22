# bmp180 Library
This library provides support for the Bosch BMP180 Digital Pressure Sensor 

This library currently supports the following platforms:
* esp-idf
* linux

Add this component to an esp-idf project with the following command:
```bash
idf.py add-dependency "zorxx/bmp180"
```

Source for this project may be found at [https://github.com/zorxx/bmp180](https://github.com/zorxx/bmp180).

# Usage

The API for this library can be found in the `include/bmp180.h` header file.

Fully-functional example applications for each supported platform can be found in the `example` directory.

Example esp-idf code-snippet:
```bash
#include "bmp180/bmp180.h"
i2c_lowlevel_config config;
config.port = I2C_NUM_0;
config.pin_sda = GPIO_NUM_21;
config.pin_scl = GPIO_NUM_22;
bmp180_t *ctx = bmp180_init(&config, DEVICE_I2C_ADDRESS, BMP180_MODE_HIGH_RESOLUTION);
if(NULL != ctx)
{
   float temperature;
   uint32_t pressure; 
   if(bmp180_measure(ctx, &temperature, &pressure))
   {
      ESP_LOGI(TAG, "Temperature: %.2f, Pressure: %u", temperature, pressure);
   }
   bmp180_free(ctx);
}
```

Example linux code-snippet:
```bash
#include "bmp180/bmp180.h"
i2c_lowlevel_config config;
config.device = "/dev/i2c-0";
bmp180_t *ctx = bmp180_init(&config, DEVICE_I2C_ADDRESS, BMP180_MODE_HIGH_RESOLUTION);
if(NULL != ctx)
{
   float temperature;
   uint32_t pressure; 

   if(bmp180_measure(ctx, &temperature, &pressure))
   {
      fprintf(stderr, "Temperature: %.2f, Pressure: %" PRIu32, temperature, pressure);
   }
   bmp180_free(ctx);
}
```

# Unit Test 

A unit test application to validate the implementation of temperature and pressure compensation calculations can be found in the `test` directory of this repository.

# License
All files delivered with this library are released under the MIT license. See the `LICENSE` file for details.
