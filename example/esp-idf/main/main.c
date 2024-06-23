/* \copyright 2024 Zorxx Software. All rights reserved.
 * \license This file is released under the MIT License. See the LICENSE file for details.
 * \brief bmp180 library esp-idf example application
 */
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "nvs_flash.h"
#include "esp_event.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "bmp180/bmp180.h"

#define TAG "BMP180"

/* The following definitions may change, based on the ESP device,
   RTC device configuration, and wiring between them. */
#define ESP_I2C_PORT I2C_NUM_0
#define ESP_I2C_SDA  GPIO_NUM_21
#define ESP_I2C_SCL  GPIO_NUM_22
#define DEVICE_I2C_ADDRESS 0 /* let the library figure it out */

void app_main(void)
{
   ESP_ERROR_CHECK(nvs_flash_init());
   ESP_ERROR_CHECK(esp_event_loop_create_default() );

   i2c_lowlevel_config config = {0};
   config.port = ESP_I2C_PORT;
   config.pin_sda = ESP_I2C_SDA;
   config.pin_scl = ESP_I2C_SCL;
   bmp180_t *ctx = bmp180_init(&config, DEVICE_I2C_ADDRESS, BMP180_MODE_HIGH_RESOLUTION);
   if(NULL == ctx)
   {
      ESP_LOGE(TAG, "Initialization failed");
   }
   else
   {
      float temperature;
      uint32_t pressure; 

      for(int i = 0; i < 100; ++i)
      {
         if(!bmp180_measure(ctx, &temperature, &pressure))
         {
            ESP_LOGE(TAG, "Query failed");
         }
         else
         {
            ESP_LOGI(TAG, "Temperature: %.2f, Pressure: %" PRIu32, temperature, pressure);
         }
         vTaskDelay(pdMS_TO_TICKS(500));
      }
      bmp180_free(ctx);
   }

   ESP_LOGI(TAG, "Test application finished");

   for(;;)
      vTaskDelay(portMAX_DELAY);
}
