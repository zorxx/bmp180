/* Copyright 2024 Zorxx Software. All rights reserved. */
#ifndef _BMP180_PRIVATE_H
#define _BMP180_PRIVATE_H

#include <stdint.h>

#if defined(BMP180_DEBUG)
   #define DBG(...) fprintf(stderr, __VA_ARGS__)
#else
   #define DBG(...)
#endif
#define MSG(...) fprintf(stderr, __VA_ARGS__)

#define ARRAY_SIZE(x) (sizeof(x)/sizeof(x[0]))

/* -----------------------------------------------------------------
 * Register Definitions
 */

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

#pragma pack(push, 2) /* 16-bit packing */
typedef struct s_bmp180_calibration_data
{
   union
   {
      uint16_t raw[11];
      struct
      {
         int16_t AC1;
         int16_t AC2;
         int16_t AC3;
         uint16_t AC4;
         uint16_t AC5;
         uint16_t AC6;
         int16_t B1;
         int16_t B2;
         int16_t MB;
         int16_t MC;
         int16_t MD;
      };
   };
} t_bmp180_calibration_data;
#pragma pack(pop)

int bmp180_Compensate(t_bmp180_calibration_data *cal, uint8_t oss,
    int32_t uncompensatedTemperature, int32_t uncompensatedPressure,
   int32_t *temperature, int32_t *pressure);

#endif /* _BMP180_PRIVATE_H */
