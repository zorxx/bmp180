/* Copyright 2024 Zorxx Software. All rights reserved. */
#include <stdio.h>
#include <inttypes.h>
#include "bmp180_private.h"

int bmp180_Compensate(t_bmp180_calibration_data *cal, uint8_t oss,
    int32_t uncompensatedTemperature, int32_t uncompensatedPressure,
   int32_t *temperature, int32_t *pressure)
{
   int32_t X1, X2, B5;
   int32_t X3, B3, B6;
   int32_t T, P;
   uint32_t B4, B7;

   SDBG("AC1 = %d", cal->AC1);
   SDBG("AC2 = %d", cal->AC2);
   SDBG("AC3 = %d", cal->AC3);
   SDBG("AC4 = %u", cal->AC4);
   SDBG("AC5 = %u", cal->AC5);
   SDBG("AC6 = %u", cal->AC6);
   SDBG("B1  = %d", cal->B1);
   SDBG("B2  = %d", cal->B2);
   SDBG("MB  = %d", cal->MB);
   SDBG("MC  = %d", cal->MC);
   SDBG("MD  = %d", cal->MD);

   SDBG("UT  = %" PRIi32, uncompensatedTemperature);
   SDBG("UP  = %" PRIi32, uncompensatedPressure);

   X1 = ((uncompensatedTemperature - (int32_t)cal->AC6) * (int32_t)cal->AC5) >> 15;
   SDBG("X1 = %" PRIi32, X1);
   X2 = (((int32_t)cal->MC) << 11) / (X1 + (int32_t)cal->MD);
   SDBG("X2 = %" PRIi32, X2);
   B5 = X1 + X2;
   SDBG("B5 = %" PRIi32, B5);
   T = (B5 + 8) >> 4;
   SDBG("T = %" PRIi32, T);
   if(NULL != temperature)
      *temperature = T;

   if(NULL != pressure)
   {
      B6 = B5 - 4000;
      SDBG("B6 = %" PRIi32, B6);
      X1 = ((int32_t)cal->B2 * ((B6 * B6) >> 12)) >> 11;
      SDBG("X1 = %" PRIi32, X1);
      X2 = ((int32_t)cal->AC2 * B6) >> 11;
      SDBG("X2 = %" PRIi32, X2);
      X3 = X1 + X2;
      SDBG("X3 = %" PRIi32, X3);

      B3 = ((((int32_t)cal->AC1 * 4 + X3) << oss) + 2) >> 2;
      SDBG("B3 = %" PRIi32, B3);
      X1 = ((int32_t)cal->AC3 * B6) >> 13;
      SDBG("X1 = %" PRIi32, X1);
      X2 = ((int32_t)cal->B1 * ((B6 * B6) >> 12)) >> 16;
      SDBG("X2 = %" PRIi32, X2);
      X3 = ((X1 + X2) + 2) >> 2;
      SDBG("X3 = %" PRIi32, X3);
      B4 = ((uint32_t)cal->AC4 * (uint32_t)(X3 + 32768)) >> 15;
      SDBG("B4 = %" PRIi32, B4);
      B7 = ((uint32_t)uncompensatedPressure - B3) * (uint32_t)(50000UL >> oss);
      SDBG("B7 = %" PRIi32, B7);

      if(B7 < 0x80000000UL)
         P = (B7 * 2) / B4;
      else
         P = (B7 / B4) * 2;

      X1 = (P >> 8) * (P >> 8);
      SDBG("X1 = %" PRIi32, X1);
      X1 = (X1 * 3038) >> 16;
      SDBG("X1 = %" PRIi32, X1);
      X2 = (-7357 * P) >> 16;
      SDBG("X2 = %" PRIi32, X2);
      P += (X1 + X2 + (int32_t)3791) >> 4;
      SDBG("P = %" PRIi32, P);

      *pressure = P;
   }

   return 0; 
}
