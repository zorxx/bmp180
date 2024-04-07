/**
 * @file bmp180.h
 * @defgroup bmp180 bmp180
 * @{
 *
 * ESP-IDF driver for BMP180 digital pressure sensor
 *
 * Ported from esp-open-rtos
 *
 * Copyright (c) 2015 Frank Bargstedt
 * Copyright (c) 2018 Ruslan V. Uss <unclerus@gmail.com>
 * Copyright (c) 2024 Zorxx Software 
 *
 * MIT Licensed as described in the file LICENSE
 */
#ifndef __BMP180_H__
#define __BMP180_H__

#include <inttypes.h>
#include <stdbool.h>
#include <driver/i2c.h>
#include <esp_err.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BMP180_DEVICE_ADDRESS 0x77 //!< I2C address

/**
 * Hardware accuracy mode.
 * See Table 3 of the datasheet
 */
typedef enum
{
    BMP180_MODE_ULTRA_LOW_POWER = 0,  //!< 1 sample, 4.5 ms
    BMP180_MODE_STANDARD,             //!< 2 samples, 7.5 ms
    BMP180_MODE_HIGH_RESOLUTION,      //!< 4 samples, 13.5 ms
    BMP180_MODE_ULTRA_HIGH_RESOLUTION //!< 8 samples, 25.5 ms
} bmp180_mode_t;

typedef void *bmp180_t;

/**
 * @brief Initialize device descriptor
 * @param port I2C port number (e.g. I2C_NUM_1)
 * @param i2c_address I2C slave address of BMP180 device (likely BMP180_DEVICE_ADDRESS)
 * @param mode query mode
 * @return bmp180_t on success, NULL on failure
 */
bmp180_t bmp180_init(i2c_port_t port, uint8_t i2c_address, bmp180_mode_t mode);

/**
 * @brief Free device descriptor
 * @param bmp obtained from a successful bmp180_init() call
 * @return `ESP_OK` on success
 */
esp_err_t bmp180_free(bmp180_t bmp);

/**
 * @brief Measure temperature and pressure
 * @param bmp obtained from a successful bmp180_init() call
 * @param[out] temperature Temperature in degrees Celsius
 * @param[out] pressure Pressure in Pa
 * @return `ESP_OK` on success
 */
esp_err_t bmp180_measure(bmp180_t bmp, float *temperature, uint32_t *pressure);

#ifdef __cplusplus
}
#endif

/**@}*/

#endif /* __BMP180_H__ */
