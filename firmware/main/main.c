/* Blink Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "sdkconfig.h"

#include "globals.h"
#include "ST7789/st7789.h"

/* PRIVATE DEFINES */
#define TAG "Main"


/*********END STATIC FUNC DECLARATIONS********/

void app_main(void)
{
    //Initialize NVS
    // ESP_ERROR_CHECK(nvs_flash_init());
    // init_spiffs();

    ST7789_Init(240,240,ROT_PORTRAIT_180);
    ST7789_Test();

    while (1)
    {
        ESP_LOGI(TAG, "Main loop");
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
}
