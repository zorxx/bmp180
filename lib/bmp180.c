/**
 * @file bmp180.c
 *
 * Cross-platform driver for BMP180 digital pressure sensor
 *
 * Ported from esp-open-rtos
 *
 * Copyright (c) 2015 Frank Bargstedt\n
 * Copyright (c) 2018 Ruslan V. Uss <unclerus@gmail.com>
 * Copyright (c) 2024 Zorxx Software
 *
 * MIT Licensed as described in the file LICENSE
 */
#include <malloc.h>
#include "bmp180/bmp180.h"
#include "bmp180_private.h"
#include "helpers.h"

#define I2C_TRANSFER_TIMEOUT  50 /* (milliseconds) give up on i2c transaction after this timeout */
#define I2C_SPEED             400000 /* hz */
#define BMP180_DELAY_BUFFER   500 /* microseconds */

typedef struct
{
   i2c_lowlevel_config i2c_config;
   i2c_lowlevel_context i2c_ctx;
   uint32_t measurement_delay;
   bmp180_mode_t mode;
   t_bmp180_calibration_data cal;
} bmp180_context_t;

static bool bmp180_get_uncompensated_temperature(bmp180_context_t *ctx, int32_t *ut)
{
   uint8_t d[2] = { BMP180_MEASURE_TEMP };
   if(!i2c_ll_write_reg(ctx->i2c_ctx, BMP180_CONTROL_REG, d, sizeof(uint8_t)))
      return false;

   sys_delay_us(4500 + BMP180_DELAY_BUFFER);
   if(!i2c_ll_read_reg(ctx->i2c_ctx, BMP180_OUT_MSB_REG, d, sizeof(d)))
      return false;
   uint32_t r = ((uint32_t)d[0] << 8) | d[1];
   *ut = r;
   SDBG("Temperature: %" PRIi32, *ut);
   return true;
}

static bool bmp180_get_uncompensated_pressure(bmp180_context_t *ctx, uint32_t *up)
{
   uint8_t oss = ctx->mode;
   uint8_t d[3] = { BMP180_MEASURE_PRESS | (oss << 6), 0 };
   if(!i2c_ll_write_reg(ctx->i2c_ctx, BMP180_CONTROL_REG, d, sizeof(uint8_t)))
      return false;
   
   sys_delay_us(ctx->measurement_delay + BMP180_DELAY_BUFFER);
   if(!i2c_ll_read_reg(ctx->i2c_ctx, BMP180_OUT_MSB_REG, d, sizeof(d)))
      return false;

   uint32_t r = ((uint32_t)d[0] << 16) | ((uint32_t)d[1] << 8) | d[2];
   r >>= 8 - oss; 
   *up = r;
   SDBG("Pressure: %" PRIu32, *up);
   return true;
}

static bool bmp180_read_calibration(bmp180_context_t *ctx)
{
   for(int i = 0; i < ARRAY_SIZE(ctx->cal.raw); ++i)
   {
      uint8_t d[] = { 0, 0 };
      uint8_t reg = BMP180_CALIBRATION_REG + (i * 2);
      if(!i2c_ll_read_reg(ctx->i2c_ctx, reg, d, sizeof(d)))
         return false;
      ctx->cal.raw[i] = ((uint16_t) d[0]) << 8 | (d[1]);
      if(ctx->cal.raw[i] == 0)
      {
         SDBG("Invalid read %u", i);
         return false; 
      }
      SDBG("Calibration value %u = %d", i, ctx->cal.raw[i]);
   }
   return true;
}

/* --------------------------------------------------------------------------------------------------------
 * Exported Functions
 */

bmp180_t bmp180_init(i2c_lowlevel_config *config, uint8_t i2c_address, bmp180_mode_t mode)
{
   bmp180_context_t *ctx;
   uint8_t id = 0;
   bool success = false;

   ctx = (bmp180_context_t *) malloc(sizeof(*ctx));
   if(NULL == ctx)
      return NULL;

   ctx->i2c_ctx = i2c_ll_init((i2c_address == 0) ? BMP180_DEVICE_ADDRESS : i2c_address,
      I2C_SPEED, I2C_TRANSFER_TIMEOUT, config);
   if(NULL == ctx->i2c_ctx)
   {
      SERR("[%s] i2c initialization failed", __func__);
      free(ctx);
      return NULL; 
   }
   ctx->mode = mode;
   switch(mode)
   {
      case BMP180_MODE_ULTRA_LOW_POWER:       ctx->measurement_delay = 4500; break;
      case BMP180_MODE_STANDARD:              ctx->measurement_delay = 7500; break;
      case BMP180_MODE_HIGH_RESOLUTION:       ctx->measurement_delay = 13500; break;
      case BMP180_MODE_ULTRA_HIGH_RESOLUTION: ctx->measurement_delay = 25500; break;
      default:
         SERR("Invalid mode %d", mode);
         free(ctx);
         return NULL; 
   }

   if(!i2c_ll_read_reg(ctx->i2c_ctx, BMP180_VERSION_REG, &id, sizeof(id))
   || id != BMP180_CHIP_ID)
   {
      SERR("Invalid device ID (0x%02x, expected 0x%02x)", id, BMP180_CHIP_ID);
   }
   else if(!bmp180_read_calibration(ctx))
   {
      SERR("Failed to read calibration");
   }
   else
   {
      SDBG("Initialization successful");
      success = true;
   }

   if(!success)
   {
      free(ctx);
      ctx = NULL;
   }
   return ctx;  
}

bool bmp180_free(bmp180_t bmp)
{
   bmp180_context_t *ctx = (bmp180_context_t *) bmp;
   if(NULL == ctx)
      return false;
   free(ctx);
   return true;
}

bool bmp180_measure(bmp180_t bmp, float *temperature, uint32_t *pressure)
{
   bmp180_context_t *ctx = (bmp180_context_t *) bmp;
   int32_t UT = 0;
   uint32_t UP = 0;
   int32_t T, P;

   if(NULL == ctx)
      return false;

   /* Temperature is always needed; required for pressure only. */
   if(!bmp180_get_uncompensated_temperature(ctx, &UT))
      return false;

   if(NULL != pressure)
   {
      if(!bmp180_get_uncompensated_pressure(ctx, &UP))
         return false;
   }

   if(bmp180_Compensate(&ctx->cal, ctx->mode, UT, UP, &T,
      (NULL == pressure) ? NULL : &P) != 0)
   {
      return false;
   }
   if(NULL != temperature)
      *temperature = (float)T/10.0;
   if(NULL != pressure)
      *pressure = P;
   return true;
}
