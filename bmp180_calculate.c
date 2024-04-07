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

   DBG("AC1 = %d\n", cal->AC1);
   DBG("AC2 = %d\n", cal->AC2);
   DBG("AC3 = %d\n", cal->AC3);
   DBG("AC4 = %u\n", cal->AC4);
   DBG("AC5 = %u\n", cal->AC5);
   DBG("AC6 = %u\n", cal->AC6);
   DBG("B1  = %d\n", cal->B1);
   DBG("B2  = %d\n", cal->B2);
   DBG("MB  = %d\n", cal->MB);
   DBG("MC  = %d\n", cal->MC);
   DBG("MD  = %d\n", cal->MD);

   DBG("UT  = %" PRIi32 "\n", uncompensatedTemperature);
   DBG("UP  = %" PRIi32 "\n", uncompensatedPressure);

   X1 = ((uncompensatedTemperature - (int32_t)cal->AC6) * (int32_t)cal->AC5) >> 15;
   DBG("X1 = %" PRIi32 "\n", X1);
   X2 = (((int32_t)cal->MC) << 11) / (X1 + (int32_t)cal->MD);
   DBG("X2 = %" PRIi32 "\n", X2);
   B5 = X1 + X2;
   DBG("B5 = %" PRIi32 "\n", B5);
   T = (B5 + 8) >> 4;
   DBG("T = %" PRIi32 "\n", T);
   if(NULL != temperature)
      *temperature = T;

   if(NULL != pressure)
   {
      B6 = B5 - 4000;
      DBG("B6 = %" PRIi32 "\n", B6);
      X1 = ((int32_t)cal->B2 * ((B6 * B6) >> 12)) >> 11;
      DBG("X1 = %" PRIi32 "\n", X1);
      X2 = ((int32_t)cal->AC2 * B6) >> 11;
      DBG("X2 = %" PRIi32 "\n", X2);
      X3 = X1 + X2;
      DBG("X3 = %" PRIi32 "\n", X3);

      B3 = ((((int32_t)cal->AC1 * 4 + X3) << oss) + 2) >> 2;
      DBG("B3 = %" PRIi32 "\n", B3);
      X1 = ((int32_t)cal->AC3 * B6) >> 13;
      DBG("X1 = %" PRIi32 "\n", X1);
      X2 = ((int32_t)cal->B1 * ((B6 * B6) >> 12)) >> 16;
      DBG("X2 = %" PRIi32 "\n", X2);
      X3 = ((X1 + X2) + 2) >> 2;
      DBG("X3 = %" PRIi32 "\n", X3);
      B4 = ((uint32_t)cal->AC4 * (uint32_t)(X3 + 32768)) >> 15;
      DBG("B4 = %" PRIi32 "\n", B4);
      B7 = ((uint32_t)uncompensatedPressure - B3) * (uint32_t)(50000UL >> oss);
      DBG("B7 = %" PRIi32 "\n", B7);

      if(B7 < 0x80000000UL)
         P = (B7 * 2) / B4;
      else
         P = (B7 / B4) * 2;

      X1 = (P >> 8) * (P >> 8);
      DBG("X1 = %" PRIi32 "\n", X1);
      X1 = (X1 * 3038) >> 16;
      DBG("X1 = %" PRIi32 "\n", X1);
      X2 = (-7357 * P) >> 16;
      DBG("X2 = %" PRIi32 "\n", X2);
      P += (X1 + X2 + (int32_t)3791) >> 4;
      DBG("P = %" PRIi32 "\n", P);

      *pressure = P;
   }

   return 0; 
}
