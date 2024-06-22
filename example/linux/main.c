/* Copyright 2024 Zorxx Software. All rights reserved. */
#include <stdbool.h>
#include <stdio.h>
#include <inttypes.h> /* PRIu32 */
#include "bmp180/bmp180.h"

#define I2C_BUS "/dev/i2c-0"
#define DEVICE_I2C_ADDRESS 0 /* let the library figure it out */

#define ERR(...) fprintf(stderr, __VA_ARGS__)
#define MSG(...) fprintf(stderr, __VA_ARGS__)

int main(int argc, char *argv[])
{
   i2c_lowlevel_config config;
   config.device = I2C_BUS;
   bmp180_t *ctx = bmp180_init(&config, DEVICE_I2C_ADDRESS, BMP180_MODE_HIGH_RESOLUTION);
   if(NULL == ctx)
   {
      ERR("Initialization failed");
   }
   else
   {
      float temperature;
      uint32_t pressure; 

      for(int i = 0; i < 100; ++i)
      {
         if(!bmp180_measure(ctx, &temperature, &pressure))
         {
            ERR("Query failed");
         }
         else
         {
            MSG("Temperature: %.2f, Pressure: %" PRIu32, temperature, pressure);
         }
         usleep(500000); 
      }
      bmp180_free(ctx);
   }

   MSG("Test application finished");
}
