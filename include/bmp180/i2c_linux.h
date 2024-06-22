#ifndef BMP180_LINUX_H
#define BMP180_LINUX_H

#include <unistd.h>

typedef struct
{
   /* Note that it may be necessary to access i2c device files as root */
   const char *device;   /* e.g. "/dev/i2c-0" */
} i2c_lowlevel_config;

#define bmp_delay_us(x) usleep(x)

#endif /* BMP180_LINUX_H */