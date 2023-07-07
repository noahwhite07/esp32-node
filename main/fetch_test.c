#include "fetch_image.h"
#include "esp_log.h"
#include "stdint.h"
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "fetch_test";

void app_main(void)
{
    xTaskCreate(init_cameras, "init_cameras", 4096, NULL, 2, NULL);
    
    // Wait for the wifi connection to initialize
    while(!cameras_initialized()){
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    ESP_LOGI(TAG, "Wifi initialized");

    // Begin downloading the image
    xTaskCreate(download_image, "download_image", 4096, NULL, 5, NULL);

    // Wait for the image to finish downloading
    while(!image_ready()){
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    ESP_LOGI(TAG, "Image downloaded");


    // Create a reference to the image buffer and fetch its size
    uint8_t * image = get_image_buffer();
    size_t imageSize = get_image_size();

    ESP_LOGI(TAG, "Image size: %zu", imageSize);
    


}