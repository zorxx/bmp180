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
#include "bmp180.h"

#define I2C_TRANSFER_TIMEOUT  50 /* (milliseconds) give up on i2c transaction after this timeout */
#define BMP180_DELAY_BUFFER   500 /* microseconds */

typedef enum
{
   CAL_AC1 = 0,
   CAL_AC2,
   CAL_AC3,
   CAL_AC4,
   CAL_AC5,
   CAL_AC6,
   CAL_B1,
   CAL_B2,
   CAL_MB,
   CAL_MC,
   CAL_MD
 } bmp_cal_data_offset_e;

#define CAL(ctx, type, val) ((type)((ctx)->cal_data[CAL_##val]))

typedef struct
{
   i2c_port_t port; 
   bool i2c_init;
   uint8_t i2c_address;
   uint32_t timeout;
   uint32_t measurement_delay;
   bmp180_mode_t mode;

   /* Calibration data: AC[1-6], B1, B2, MB, MC, MD */
   uint16_t cal_data[11];
} bmp180_context_t;

#define BMP180_VERSION_REG        0xD0
#define BMP180_CONTROL_REG        0xF4
#define BMP180_RESET_REG          0xE0
#define BMP180_OUT_MSB_REG        0xF6
#define BMP180_OUT_LSB_REG        0xF7
#define BMP180_OUT_XLSB_REG       0xF8

#define BMP180_CALIBRATION_REG    0xAA

/* Values for BMP180_CONTROL_REG */
#define BMP180_MEASURE_TEMP       0x2E
#define BMP180_MEASURE_PRESS      0x34

/* CHIP ID stored in BMP180_VERSION_REG */
#define BMP180_CHIP_ID            0x55

/* Reset value for BMP180_RESET_REG */
#define BMP180_RESET_VALUE        0xB6

#define ARRAY_SIZE(x) (sizeof(x)/sizeof(x[0]))

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
   ESP_LOGI(__func__, "Temperature: %" PRIi32, *ut);
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
   ESP_LOGI(__func__, "Pressure: %" PRIu32, *up);
   return ESP_OK;
}

static esp_err_t bmp180_read_calibration(bmp180_context_t *ctx)
{
   esp_err_t result;

   for(int i = 0; i < ARRAY_SIZE(ctx->cal_data); ++i)
   {
      uint8_t d[] = { 0, 0 };
      uint8_t reg = BMP180_CALIBRATION_REG + (i * 2);
      result = i2c_read(ctx, reg, d, sizeof(d));
      if(ESP_OK != result)
         return result;
      ctx->cal_data[i] = ((uint16_t) d[0]) << 8 | (d[1]);
      if(ctx->cal_data[i] == 0)
      {
         ESP_LOGI(__func__, "Invalid read %u", i);
         return ESP_ERR_INVALID_RESPONSE;
      }
      ESP_LOGI(__func__, "Calibration value %u = %d", i, ctx->cal_data[i]);
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
   uint8_t oss = ctx->mode; /* oversample setting */

   if(NULL == ctx)
      return ESP_FAIL;

   // Temperature is always needed, also required for pressure only.

   int32_t T, P;
   int32_t UT, X1, X2, B5;
   UT = 0;
   result = bmp180_get_uncompensated_temperature(ctx, &UT);
   if(ESP_OK != result)
      return result;

   X1 = ((UT - (int32_t)CAL(ctx, uint16_t, AC6)) * (int32_t)CAL(ctx, uint16_t, AC5)) >> 15;
   X2 = (((int32_t)CAL(ctx, int16_t, MC)) << 11) / (X1 + (int32_t)CAL(ctx, int16_t, MD));
   B5 = X1 + X2;
   T = (B5 + 8) >> 4;

   if(NULL != temperature)
      *temperature = T / 10.0;

   ESP_LOGD(__func__, "T:= %" PRIi32 ".%d", T / 10, abs(T % 10));

   if(NULL != pressure)
   {
      int32_t X3, B3, B6;
      uint32_t B4, B7, UP = 0;

      result = bmp180_get_uncompensated_pressure(ctx, &UP);
      if(ESP_OK != result)
         return result;

      B6 = B5 - 4000;
      X1 = ((int32_t)CAL(ctx, int16_t, B2) * ((B6 * B6) >> 12)) >> 11;
      X2 = ((int32_t)CAL(ctx, int16_t, AC2) * B6) >> 11;
      X3 = X1 + X2;

      B3 = ((((int32_t)CAL(ctx, int16_t, AC1) * 4 + X3) << oss) + 2) >> 2;
      X1 = ((int32_t)CAL(ctx, int16_t, AC3) * B6) >> 13;
      X2 = ((int32_t)CAL(ctx, int16_t, B1) * ((B6 * B6) >> 12)) >> 16;
      X3 = ((X1 + X2) + 2) >> 2;
      B4 = ((uint32_t)CAL(ctx, uint16_t, AC4) * (uint32_t)(X3 + 32768)) >> 15;
      B7 = ((uint32_t)UP - B3) * (uint32_t)(50000UL >> oss);

      if (B7 < 0x80000000UL)
         P = (B7 * 2) / B4;
      else
         P = (B7 / B4) * 2;

      X1 = (P >> 8) * (P >> 8);
      X1 = (X1 * 3038) >> 16;
      X2 = (-7357 * P) >> 16;
      P = P + ((X1 + X2 + (int32_t)3791) >> 4);

      *pressure = P;

      ESP_LOGD(__func__, "P:= %" PRIi32, P);
   }

   return ESP_OK;
}
