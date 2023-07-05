#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "stdint.h"

#include "ultrasonic_sensors.h"
#include "esp32_pin_aliases.h"
#include "node_sensor_config.h"

static const char *TAG = "read_sensor";

void on_camera_triggered();

void app_main(void){

    // // Pass in a callback function to the xtask
    // task_params_t params;
    // params.callback_function = NULL;

    // sensor_pair_t s4 = {
    //     .trig =  S4_TRIG,
    //     .echo_a = S4_ECHO_A,
    //     .echo_b = S4_ECHO_B,
    //     .zone = 1
    // };

    sensor_pair_t *s4 = (sensor_pair_t *)malloc(sizeof(sensor_pair_t));
    s4->trig =  S4_TRIG;
    s4->echo_a = S4_ECHO_A;
    s4->echo_b = S4_ECHO_B;
    s4->zone = 1;


    // register_threshold_params_t params = {
    //     .sensor_pair = s4,
    //     .callback_function = on_camera_triggered
    // };

    register_threshold_params_t *params = (register_threshold_params_t *)malloc(sizeof(register_threshold_params_t));
    params->sensor_pair = *s4;
    params->callback_function = on_camera_triggered;

    //xTaskCreate(ultrasonic_test, "ultrasonic_test", 4096, &params, 2, NULL);
    xTaskCreate(register_threshold, "register_threshold", 2048, params, 2, NULL);

    
}

// A callback function for when the camera has triggered
void on_camera_triggered(){
    ESP_LOGI(TAG, "Camera triggered callback");
}