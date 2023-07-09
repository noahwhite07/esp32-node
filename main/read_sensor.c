#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "stdint.h"

#include "ultrasonic_sensors.h"
#include "esp32_pin_aliases.h"
#include "node_sensor_config.h"

static const char *TAG = "read_sensor";

void on_camera_triggered(uint8_t);

void app_main(void){

    //========================================================//
    // Start polling sensor 1
    //========================================================//

    ESP_LOGI(TAG, "Initializing sensor pair 1");
    sensor_pair_t *s1 = (sensor_pair_t *)malloc(sizeof(sensor_pair_t));
    s1->trig =  S1_TRIG;
    s1->echo_a = S1_ECHO_A;
    s1->echo_b = S1_ECHO_B;
    s1->zone = 1;

    register_threshold_params_t *s1_params = (register_threshold_params_t *)malloc(sizeof(register_threshold_params_t));
    s1_params->sensor_pair = *s1;
    s1_params->callback_function = on_camera_triggered;

    xTaskCreate(register_threshold, "register_threshold", 2048, s1_params, 4, NULL);

    //========================================================//
    // Start polling sensor 2
    //========================================================//

    sensor_pair_t *s2 = (sensor_pair_t *)malloc(sizeof(sensor_pair_t));
    s2->trig =  S2_TRIG;
    s2->echo_a = S2_ECHO_A;
    s2->echo_b = S2_ECHO_B;
    s2->zone = 2;

    register_threshold_params_t *s2_params = (register_threshold_params_t *)malloc(sizeof(register_threshold_params_t));
    s2_params->sensor_pair = *s2;
    s2_params->callback_function = on_camera_triggered;

    xTaskCreate(register_threshold, "register_threshold", 2048, s2_params, 4, NULL);

    //========================================================//
    // Start polling sensor 3
    //========================================================//

    sensor_pair_t *s3 = (sensor_pair_t *)malloc(sizeof(sensor_pair_t));
    s3->trig =  S3_TRIG;
    s3->echo_a = S3_ECHO_A;
    s3->echo_b = S3_ECHO_B;
    s3->zone = 3;

    register_threshold_params_t *s3_params = (register_threshold_params_t *)malloc(sizeof(register_threshold_params_t));
    s3_params->sensor_pair = *s3;
    s3_params->callback_function = on_camera_triggered;
        
    //xTaskCreate(register_threshold, "register_threshold", 2048, s3_params, 4, NULL);

    //========================================================//
    // Start polling sensor 4
    //========================================================//

    sensor_pair_t *s4 = (sensor_pair_t *)malloc(sizeof(sensor_pair_t));
    s4->trig =  S4_TRIG;
    s4->echo_a = S4_ECHO_A;
    s4->echo_b = S4_ECHO_B;
    s4->zone = 4;
    
    register_threshold_params_t *s4_params = (register_threshold_params_t *)malloc(sizeof(register_threshold_params_t));
    s4_params->sensor_pair = *s4;
    s4_params->callback_function = on_camera_triggered;

    //xTaskCreate(register_threshold, "register_threshold", 2048, s4_params, 4, NULL);

}

/*
    A callback function for when the camera has triggered

    Takes in the number of the zone that was crossed into
*/ 

void on_camera_triggered(uint8_t zone_number){
    ESP_LOGI(TAG, "************************");
    ESP_LOGI(TAG, "Zone %d entered", zone_number);
    ESP_LOGI(TAG, "************************");


}