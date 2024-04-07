/* Copyright 2024 Zorxx Software. All rights reserved. */
#include <stdbool.h>
#include <stdio.h>
#include <inttypes.h>
#include "bmp180_private.h"

typedef struct
{
   t_bmp180_calibration_data cal;
   uint8_t oss;
   int32_t uncompensatedTemperature;
   int32_t uncompensatedPressure;
   int32_t resultTemperature;
   int32_t resultPressure;
} t_test_vector;

static t_test_vector test_vectors[] =
{
   /* test vector, from the datasheet */
   { { 408, -72, -14383, 32741, 32757, 23153, 6190, 4, -32768, -8711, 2868 },   
     0,        /* oss */
     27898L,   /* uncompensated temperature */
     23843L,   /* uncompensated pressure */
     150L,     /* result (compensated) temperature */
     69964L }, /* result (compensated) pressure */

   /* test vector, from device */ 
   { { 8962, -1194, -14683, 34018, 25305, 17872, 6515, 48, -32768, -11786, 2634 },   
     0,        /* oss */
     26042L,   /* uncompensated temperature */
     42852L,   /* uncompensated pressure */
     226L,     /* result (compensated) temperature, in degrees Celsius * 10 */
     98900L }, /* result (compensated) pressure, in pascals */

   { { 8962, -1194, -14683, 34018, 25305, 17872, 6515, 48, -32768, -11786, 2634 },   
     2,        /* oss */
     25971L,   /* uncompensated temperature */
     170392L,  /* uncompensated pressure */
     221L,     /* result (compensated) temperature, in degrees Celsius * 10 */
     98032L }, /* result (compensated) pressure, in pascals */
};

int main(int argc, char *argv[])
{
    size_t vector_count = ARRAY_SIZE(test_vectors);
    int32_t temperature, pressure;
    bool success = true;

    for(size_t i = 0; i < vector_count; ++i)
    {
        t_test_vector *v = &test_vectors[i];
        if(bmp180_Compensate(&v->cal, v->oss, v->uncompensatedTemperature,
           v->uncompensatedPressure, &temperature, &pressure) != 0)
        {
            MSG("Failed to perform compensation for vector %zu of %zu\n", i+1, vector_count);
            success = false;
        }
        else
        {
            if(temperature != v->resultTemperature)
            {
                MSG("Temperature mismatch: expected %" PRIi32 ", received %" PRIi32 "\n",
                   v->resultTemperature, temperature);
                success = false;
            }
            if(pressure != v->resultPressure)
            {
                MSG("Pressure mismatch: expected %" PRIi32 ", received %" PRIi32 "\n",
                   v->resultPressure, pressure);
                success = false;
            }
        }

        if(success)
        {
            float c = (float)temperature/10.0;
            float mmHg = (float)pressure*0.00750062;
            float inHg = (float)pressure*0.0002953;
            MSG("Test case %zu success: temperature %.2f C, pressure %u pascal, %.2f mmHg. %.2f inHg\n", i+1, c, pressure, mmHg, inHg); 
        }
    }
    return 0;
}
