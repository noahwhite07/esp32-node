#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "stdint.h"

#include "ultrasonic_sensors.h"
#include "esp32_pin_aliases.h"

static const char *TAG = "read_sensor";

void on_camera_triggered();

void app_main(void){

    // Pass in a callback function to the xtask
    task_params_t params;
    params.callback_function = on_camera_triggered;


    xTaskCreate(ultrasonic_test, "ultrasonic_test", 4096, &params, 2, NULL);
    ESP_LOGI(TAG, "LED on GPIO %d", PIN_LED);
    
}

// A callback function for when the camera has triggered
void on_camera_triggered(){
    ESP_LOGI(TAG, "Camera triggered callback");
}