/**
 * @file bmp180.c
 *
 * ESP-IDF driver for BMP180 digital pressure sensor
 *
 * Ported from esp-open-rtos
 *
 * Copyright (c) 2015 Frank Bargstedt\n
 * Copyright (c) 2018 Ruslan V. Uss <unclerus@gmail.com>
 * Copyright (c) 2024 Zorxx Software
 *
 * MIT Licensed as described in the file LICENSE
 */
#include <inttypes.h>
#include <esp_err.h>
#include <esp_log.h>
#include <rom/ets_sys.h>
#include "bmp180_private.h"
#include "bmp180.h"

#define I2C_TRANSFER_TIMEOUT  50 /* (milliseconds) give up on i2c transaction after this timeout */
#define BMP180_DELAY_BUFFER   500 /* microseconds */

typedef struct
{
   i2c_port_t port; 
   bool i2c_init;
   uint8_t i2c_address;
   uint32_t timeout;
   uint32_t measurement_delay;
   bmp180_mode_t mode;
   t_bmp180_calibration_data cal;
} bmp180_context_t;

static __inline esp_err_t i2c_read(bmp180_context_t *ctx, uint8_t reg, uint8_t *data, uint8_t length)
{
   return i2c_master_write_read_device((ctx)->port, (ctx)->i2c_address,
      &reg, sizeof(reg), data, length, (ctx)->timeout);
}
#define i2c_write(ctx, data, length)                           \
   i2c_master_write_to_device((ctx)->port, (ctx)->i2c_address, \
      data, length, (ctx)->timeout)

static esp_err_t bmp180_get_uncompensated_temperature(bmp180_context_t *ctx, int32_t *ut)
{
   uint8_t d[] = { BMP180_CONTROL_REG, BMP180_MEASURE_TEMP };
   esp_err_t result = i2c_write(ctx, d, sizeof(d));
   if(result != ESP_OK)
      return result;
   ets_delay_us(4500 + BMP180_DELAY_BUFFER);
   result = i2c_read(ctx, BMP180_OUT_MSB_REG, d, sizeof(d)); 
   if(result != ESP_OK)
      return result;
   uint32_t r = ((uint32_t)d[0] << 8) | d[1];
   *ut = r;
   ESP_LOGD(__func__, "Temperature: %" PRIi32, *ut);
   return ESP_OK;
}

static esp_err_t bmp180_get_uncompensated_pressure(bmp180_context_t *ctx, uint32_t *up)
{
   uint8_t oss = ctx->mode;
   uint8_t d[] = { BMP180_CONTROL_REG, BMP180_MEASURE_PRESS | (oss << 6), 0 };
   esp_err_t result = i2c_write(ctx, d, sizeof(d));
   if(ESP_OK != result)
      return result;
   ets_delay_us(ctx->measurement_delay + BMP180_DELAY_BUFFER);
   result = i2c_read(ctx, BMP180_OUT_MSB_REG, d, sizeof(d));
   if(ESP_OK != result)
      return result;

   uint32_t r = ((uint32_t)d[0] << 16) | ((uint32_t)d[1] << 8) | d[2];
   r >>= 8 - oss; 
   *up = r;
   ESP_LOGD(__func__, "Pressure: %" PRIu32, *up);
   return ESP_OK;
}

static esp_err_t bmp180_read_calibration(bmp180_context_t *ctx)
{
   esp_err_t result;

   for(int i = 0; i < ARRAY_SIZE(ctx->cal.raw); ++i)
   {
      uint8_t d[] = { 0, 0 };
      uint8_t reg = BMP180_CALIBRATION_REG + (i * 2);
      result = i2c_read(ctx, reg, d, sizeof(d));
      if(ESP_OK != result)
         return result;
      ctx->cal.raw[i] = ((uint16_t) d[0]) << 8 | (d[1]);
      if(ctx->cal.raw[i] == 0)
      {
         ESP_LOGI(__func__, "Invalid read %u", i);
         return ESP_ERR_INVALID_RESPONSE;
      }
      ESP_LOGI(__func__, "Calibration value %u = %d", i, ctx->cal.raw[i]);
   }

   return ESP_OK;
}

/* --------------------------------------------------------------------------------------------------------
 * Exported Functions
 */

bmp180_t bmp180_init(i2c_port_t port, uint8_t i2c_address, bmp180_mode_t mode)
{
   bmp180_context_t *ctx;
   esp_err_t result;
   uint8_t id = 0;

   ctx = (bmp180_context_t *) calloc(1, sizeof(*ctx));
   if(NULL == ctx)
      return NULL; 

   ctx->i2c_address = i2c_address;
   ctx->port = port;
   ctx->mode = mode;
   ctx->timeout = pdMS_TO_TICKS(I2C_TRANSFER_TIMEOUT);
   switch(mode)
   {
      case BMP180_MODE_ULTRA_LOW_POWER:       ctx->measurement_delay = 4500; break;
      case BMP180_MODE_STANDARD:              ctx->measurement_delay = 7500; break;
      case BMP180_MODE_HIGH_RESOLUTION:       ctx->measurement_delay = 13500; break;
      case BMP180_MODE_ULTRA_HIGH_RESOLUTION: ctx->measurement_delay = 25500; break;
      default:
         ESP_LOGE(__func__, "Invalid mode %d", mode);
         free(ctx);
         return NULL; 
   }

   result = i2c_read(ctx, BMP180_VERSION_REG, &id, sizeof(id));
   if(result != ESP_OK || id != BMP180_CHIP_ID)
   {
      ESP_LOGE(__func__, "Invalid device ID (0x%02x, expected 0x%02x)", id, BMP180_CHIP_ID);
      if(result == ESP_OK)
         result = ESP_ERR_INVALID_RESPONSE;
   }
   else
   {
      result = bmp180_read_calibration(ctx);
      if(ESP_OK != result)
      {
         ESP_LOGE(__func__, "Failed to read calibration");
      }
   }

   if(result != ESP_OK)
   {
      free(ctx);
      ctx = NULL;
   }

   ESP_LOGI(__func__, "Initialization successful");
   return ctx;  
}

esp_err_t bmp180_free_desc(bmp180_t bmp)
{
   bmp180_context_t *ctx = (bmp180_context_t *) bmp;
   if(NULL == ctx)
      return ESP_FAIL;

   free(ctx);
   return ESP_OK;
}

esp_err_t bmp180_measure(bmp180_t bmp, float *temperature, uint32_t *pressure)
{
   bmp180_context_t *ctx = (bmp180_context_t *) bmp;
   esp_err_t result;
   int32_t UT = 0;
   uint32_t UP = 0;
   int32_t T, P;

   if(NULL == ctx)
      return ESP_FAIL;

   /* Temperature is always needed; required for pressure only. */
   result = bmp180_get_uncompensated_temperature(ctx, &UT);
   if(ESP_OK != result)
      return result;

   if(NULL != pressure)
   {
      result = bmp180_get_uncompensated_pressure(ctx, &UP);
      if(ESP_OK != result)
         return result;
   }

   if(bmp180_Compensate(&ctx->cal, ctx->mode, UT, UP, &T,
      (NULL == pressure) ? NULL : &P) != 0)
   {
      return ESP_FAIL;
   }
   if(NULL != temperature)
      *temperature = (float)T/10.0;
   if(NULL != pressure)
      *pressure = P;
   return ESP_OK;
}
