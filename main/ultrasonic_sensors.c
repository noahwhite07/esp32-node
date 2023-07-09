#include <stdio.h>
#include <stdbool.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <ultrasonic.h>
#include <esp_err.h>
#include <float.h>
#include "esp_log.h"

#include "ultrasonic_sensors.h"
#include "esp32_pin_aliases.h"
#include "node_sensor_config.h"

#define ANSI_COLOR_RED     "\x1b[31m"

#define MAX_DISTANCE_CM 500 // 5m max - 16 feet

// For the implementation of the state machine that detects vehicles
#define STATE_NONE_TRIPPED 0
#define STATE_A_TRIPPED 1
#define STATE_BOTH_TRIPPED 2
#define STATE_B_TRIPPED 3

static const char *TAG = "ultrasonic_sensors";


void register_threshold(void *pvParameters){
    // Cast pvParameters back to the appropriate type
    register_threshold_params_t *params = (register_threshold_params_t *)pvParameters;

    // Extract the callback function to call when the threshold is crossed
    callback_t thresh_trip_callback = params->callback_function;

    // Extract the sensor pair corresponding to this threshold
    sensor_pair_t ultrasonic_pair = params->sensor_pair;

    // Get the GPIO numbers of the trigger, echo_a, and echo_b pins
    uint8_t trig_gpio = ultrasonic_pair.trig;
    uint8_t echo_a_gpio = ultrasonic_pair.echo_a;
    uint8_t echo_b_gpio = ultrasonic_pair.echo_b;

    // Get the zone to which this threshold is the entrance
    uint8_t zone_num = ultrasonic_pair.zone;


    //printf("Pinout for sensor A: trig: %d, echo_a: %d, echo_b: %d", trig_gpio, echo_a_gpio, echo_b_gpio);
    //printf("trig: %d, echo_a: %d, echo_b: %d", trig_gpio, echo_a_gpio, echo_b_gpio);
    // Structs for passing the sensors into the ultrasonic library
    ultrasonic_sensor_t sensor_a = {
        .trigger_pin = trig_gpio,
        .echo_pin = echo_a_gpio
    };

    ultrasonic_sensor_t sensor_b = {
        .trigger_pin = trig_gpio,
        .echo_pin = echo_b_gpio
    };

    
    // Initialize the sensors with the ultrasonic library
    // TODO: add error checking to init
    ultrasonic_init(&sensor_a);
    ultrasonic_init(&sensor_b);

    // Delay after each sensor measurement
    const TickType_t sensorDelay = pdMS_TO_TICKS(40);
    
    // The measurment distance (in cm) at which a sensor is considered "crossed"
    uint16_t threshold_distance = 50; 

    // Hold the most recent measurment from each sensor
    float distance_a, distance_b;

    // The initial state of the sensor pair
    int state = STATE_NONE_TRIPPED;

    ESP_LOGI(TAG, "Sensor pair %d registered. Started polling sensors...", zone_num);

    // Constantly poll the sensor pair at a regular interval
    while(true){

        // Store the previous state of the sensor pair
        int prev_state = state;

        // Take a measurment from each sensor
        esp_err_t res_a = ultrasonic_measure(&sensor_a, MAX_DISTANCE_CM, &distance_a);
        vTaskDelay(sensorDelay); // Introduce delay before triggering sensor A

        esp_err_t res_b = ultrasonic_measure(&sensor_b, MAX_DISTANCE_CM, &distance_b);
        vTaskDelay(sensorDelay); // Introduce delay before triggering sensor B

        // Convert reading from meters to centimeters
        distance_a *= 100;
        distance_b *= 100;

        // Print the measurements from each sensor
        if (res_a != ESP_OK){
            printf("Sensor A - Error %d\n", res_a);   
            distance_a = FLT_MAX;
        } else{
            //printf("Sensor %dA - Distance: %.2f cm\n", zone_num, distance_a);      
        }

        if (res_b != ESP_OK){
            distance_b = FLT_MAX;
            printf("Sensor B - Error %d\n", res_b); 
        } else {
            //printf("Sensor %dB - Distance: %.2f cm\n", zone_num, distance_b );
        }


        // Determine the current state of the pair based on the most recent measurments
        if (distance_a < threshold_distance && distance_b < threshold_distance) {
            state = STATE_BOTH_TRIPPED; // 2
        } else if (distance_a < threshold_distance) {
            state = STATE_A_TRIPPED; // 1
        } else if (distance_b < threshold_distance) {
            state = STATE_B_TRIPPED; //3
        } else {
            state = STATE_NONE_TRIPPED; //0
        }

        // Detection sequence should go 0-->1-->2-->3-->0

        if(prev_state != state){
            ESP_LOGI(TAG, "Sensor %d state transition.\tPrev state: %d, New state: %d", zone_num, prev_state, state);
            printf("Distance A: %.2f\tDistance B: %.2f\t Threshold Distance: %d", distance_a, distance_b, threshold_distance);
        }
        // Detect a valid transition sequence (A -> BOTH -> B)
        if (prev_state == STATE_A_TRIPPED && state == STATE_BOTH_TRIPPED) {

            // No action needed, just continue to the next state
        } else if(prev_state == STATE_NONE_TRIPPED && state == STATE_A_TRIPPED){
            // Transition from none to A is also valid, so no action needed

        }else if (prev_state == STATE_BOTH_TRIPPED && state == STATE_B_TRIPPED) {

            // We've detected a valid sequence, so call the callback
            printf("Distance A: %.2f\tDistance B: %.2f\t Threshold Distance: %d", distance_a, distance_b, threshold_distance);
            thresh_trip_callback(zone_num);

            // Prevent the threshold from being continuously tripped by a passing vehicle
            vTaskDelay(pdMS_TO_TICKS(5000));

        } else if (state != prev_state) {
            ESP_LOGI(TAG, "Invalid sequence, resetting state...");

            // If we're not in a valid sequence and the state has changed, reset the sequence
            state = STATE_NONE_TRIPPED;
        }

        

        

        // TODO: Refactor this detection logic 
        // if(distance_a < threshold_distance && distance_b < threshold_distance){
        //     // Alert the caller of this function that the threshold has been tripped
        //     printf("Distance A: %.2f\tDistance B: %.2f\t Threshold Distance: %d", distance_a, distance_b, threshold_distance);
        //     thresh_trip_callback(zone_num);

        //     // Prevent the threshold from being continuously tripped by a passing vehicle
        //     vTaskDelay(pdMS_TO_TICKS(5000)); //TODO: Do this in a more elegant way
        // }

    }

    // This will be unreachable for now
    //free(pvParameters)

}