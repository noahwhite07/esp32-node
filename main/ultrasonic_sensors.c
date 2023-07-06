#include <stdio.h>
#include <stdbool.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <ultrasonic.h>
#include <esp_err.h>
#include <float.h>

#include "ultrasonic_sensors.h"
#include "esp32_pin_aliases.h"
#include "node_sensor_config.h"

#define ANSI_COLOR_RED     "\x1b[31m"

#define MAX_DISTANCE_CM 500 // 5m max - 16 feet


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

    // Something is going wrong here. These variables are just reading garbage. Maybe they should be malloced
    printf("Pinout for sensor A: trig: %d, echo_a: %d, echo_b: %d", trig_gpio, echo_a_gpio, echo_b_gpio);
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
    const TickType_t sensorDelay = pdMS_TO_TICKS(200);
    
    // The measurment distance (in cm) at which a sensor is considered "crossed"
    uint16_t threshold_distance = 50; 

    // Hold the most recent measurment from each sensor
    float distance_a, distance_b;

    // Constantly poll the sensor pair at a regular interval
    while(true){

        // Take a measurment from each sensor
        esp_err_t res_a = ultrasonic_measure(&sensor_a, MAX_DISTANCE_CM, &distance_a);
        vTaskDelay(sensorDelay*2); // Introduce delay before triggering sensor A

        esp_err_t res_b = ultrasonic_measure(&sensor_b, MAX_DISTANCE_CM, &distance_b);
        vTaskDelay(sensorDelay*2); // Introduce delay before triggering sensor B

        // Convert reading from meters to centimeters
        distance_a *= 100;
        distance_b *= 100;

        // Print the measurements from each sensor
        if (res_a != ESP_OK){
            printf("Sensor A - Error %d\n", res_a);   
            distance_a = FLT_MAX;
        } else{
            printf("Sensor %dA - Distance: %.2f cm\n", zone_num, distance_a);      
        }

        if (res_b != ESP_OK){
            distance_b = FLT_MAX;
            printf("Sensor B - Error %d\n", res_b); 
        } else {
            printf("Sensor %dB - Distance: %.2f cm\n", zone_num, distance_b );
        }

        // TODO: Refactor this detection logic 
        if(distance_a < threshold_distance && distance_b < threshold_distance){
            // Alert the caller of this function that the threshold has been tripped
            printf("Distance A: %.2f\tDistance B: %.2f\t Threshold Distance: %d", distance_a, distance_b, threshold_distance);
            thresh_trip_callback(zone_num);

            // Prevent the threshold from being continuously tripped by a passing vehicle
            vTaskDelay(pdMS_TO_TICKS(5000)); //TODO: Do this in a more elegant way
        }

    }

    // This will be unreachable for now
    //free(pvParameters)

}

// Deprecated
// void ultrasonic_test(void *pvParameters)
// {
//     // Cast pvParameters back to the appropriate type
//     task_params_t *params = (task_params_t *)pvParameters;
    
//     // Extract the callback function for when 
//     callback_t camera_trigger_callback = params->callback_function;



//     ultrasonic_sensor_t sensor1 = {
//         .trigger_pin = TRIGGER_GPIO,
//         .echo_pin = ECHO_GPIO_1
//     };

//     ultrasonic_sensor_t sensor2 = {
//         .trigger_pin = TRIGGER_GPIO,
//         .echo_pin = ECHO_GPIO_2
//     };

    

//     // ultrasonic_sensor_t sensor3 = {
//     //     .trigger_pin = TRIGGER_GPIO,
//     //     .echo_pin = ECHO_GPIO_3
//     // };

//     // ultrasonic_sensor_t sensor4 = {
//     //     .trigger_pin = TRIGGER_GPIO,
//     //     .echo_pin = ECHO_GPIO_4
//     // };

//     // ultrasonic_sensor_t sensor5 = {
//     //     .trigger_pin = TRIGGER_GPIO,
//     //     .echo_pin = ECHO_GPIO_5
//     // };

//     // ultrasonic_sensor_t sensor6 = {
//     //     .trigger_pin = TRIGGER_GPIO,
//     //     .echo_pin = ECHO_GPIO_6
//     // };

//     // ultrasonic_sensor_t sensor7 = {
//     //     .trigger_pin = TRIGGER_GPIO,
//     //     .echo_pin = ECHO_GPIO_7
//     // };

//     // ultrasonic_sensor_t sensor8 = {
//     //     .trigger_pin = TRIGGER_GPIO,
//     //     .echo_pin = ECHO_GPIO_8
//     // };

//     // TODO: add error checking to init
//     ultrasonic_init(&sensor1);
//     ultrasonic_init(&sensor2);

//     // ultrasonic_init(&sensor3);
//     // ultrasonic_init(&sensor4);
//     // ultrasonic_init(&sensor5);
//     // ultrasonic_init(&sensor6);
//     // ultrasonic_init(&sensor7);
//     // ultrasonic_init(&sensor8);

//     //activate camera, start as off
//     bool camera1 = false;
//     // bool camera2 = false;
//     // bool camera3 = false;
//     // bool camera4 = false; //echo 32 trigger 33


//     // delay after each sensor measurement
//     const TickType_t sensorDelay = pdMS_TO_TICKS(100);
//     // speed_of_sound = 331.4 + (0.6 * temperature)

//     //1 foot = 30.48 centimeters
//     //threshold vehicle detection distance
//     int THRESHOLD_DISTANCE = 200; // 10 cm for testing library

//     while (true)
//     {

        
//         float distance1 , distance2 /*, distance3, distance4, distance5, distance6, distance7, distance8*/;

//         esp_err_t res1 = ultrasonic_measure(&sensor1, MAX_DISTANCE_CM, &distance1);
//         vTaskDelay(sensorDelay*2); // Introduce delay before triggering sensor 2

//         esp_err_t res2 = ultrasonic_measure(&sensor2, MAX_DISTANCE_CM, &distance2);
//         vTaskDelay(sensorDelay*2); // Introduce delay before triggering sensor 3

//         // esp_err_t res3 = ultrasonic_measure(&sensor3, MAX_DISTANCE_CM, &distance3);
//         // vTaskDelay(sensorDelay); // Introduce delay before triggering sensor 4

//         // esp_err_t res4 = ultrasonic_measure(&sensor4, MAX_DISTANCE_CM, &distance4);
//         // vTaskDelay(sensorDelay); // Introduce delay before triggering sensor 5

//         // esp_err_t res5 = ultrasonic_measure(&sensor5, MAX_DISTANCE_CM, &distance5);
//         // vTaskDelay(sensorDelay); // Introduce delay before triggering sensor 6

//         // esp_err_t res6 = ultrasonic_measure(&sensor6, MAX_DISTANCE_CM, &distance6);
//         // vTaskDelay(sensorDelay); // Introduce delay before triggering sensor 7

//         // esp_err_t res7 = ultrasonic_measure(&sensor7, MAX_DISTANCE_CM, &distance7);
//         // vTaskDelay(sensorDelay); // Introduce delay before triggering sensor 8

//         // esp_err_t res8 = ultrasonic_measure(&sensor8, MAX_DISTANCE_CM, &distance8);

//         //for each sensor, display distance if no error
//         if (res1 != ESP_OK)
//         {
//             printf("Sensor 1 - Error %d\n", res1);
            
//             /* 
//                 If the sensor exceeds its range, set distance to 
//                 arbitrarily large value so logic can continue
//             */
//             distance1 = FLT_MAX;
//         }
//         else
//         {
//             printf("Sensor 1 - Distance: %.2f cm\n", distance1 * 100);
            
//         }

//         if (res2 != ESP_OK)
//         {
//             /* 
//                 If the sensor exceeds its range, set distance to 
//                 arbitrarily large value so logic can continue
//             */
//             distance2 = FLT_MAX;
//             printf("Sensor 2 - Error %d\n", res2);
          
//         }
//         else
//         {
//             printf("Sensor 2 - Distance: %.2f cm\n", distance2 * 100);
//         }

        

//         // if (res3 != ESP_OK)
//         // {
//         //     printf("Sensor 3 - Error %d\n", res3);
//         // }
//         // else
//         // {
//         //     printf( "Sensor 3 - Distance: %.2f cm\n", distance3 * 100);
//         // }

//         // if (res4 != ESP_OK)
//         // {
//         //     printf( "Sensor 4 - Error %d\n", res4);
//         // }
//         // else
//         // {
//         //     printf("Sensor 4 - Distance: %.2f cm\n", distance4 * 100);
//         // }

//         // if (res5 != ESP_OK)
//         // {
//         //     printf("Sensor 5 - Error %d\n", res5);
//         // }
//         // else
//         // {
//         //     printf("Sensor 5 - Distance: %.2f cm\n", distance5 * 100);
//         // }

//         // if (res6 != ESP_OK)
//         // {
//         //     printf("Sensor 6 - Error %d\n", res6);
//         // }
//         // else
//         // {
//         //     printf("Sensor 6 - Distance: %.2f cm\n", distance6 * 100);
//         // }

//         // if (res7 != ESP_OK)
//         // {
//         //     printf("Sensor 7 - Error %d\n", res7);
//         // }
//         // else
//         // {
//         //     printf("Sensor 7 - Distance: %.2f cm\n", distance7 * 100);
//         // }

//         // if (res8 != ESP_OK)
//         // {
//         //     printf("Sensor 8 - Error %d\n", res8);
//         // }
//         // else
//         // {
//         //     printf("Sensor 8 - Distance: %.2f cm\n", distance8 * 100);
//         // }

    

//        // if distance for sensor 1 is greater than threshold, vehicle entering zone
//         if(distance1  * 100 > THRESHOLD_DISTANCE){
          
//           if(camera1 == true){

//             printf("Car ENTERING zone 1\n");

//           }
//             camera1 = false;

//         }
//        // if distance for sensor 2 is greater than threshold, vehicle leaving zone
//         if(distance2 * 100 > THRESHOLD_DISTANCE){
            
//             if(camera1 == true){

//             printf("Car LEAVING ZONE 1\n");

//           }
//             camera1 = false;
//         }

//         //if both sensors are less than threshold, vehicle between sensors --> turn on camera
//         if(distance1 * 100 <THRESHOLD_DISTANCE && distance2 * 100 < THRESHOLD_DISTANCE && camera1 == false){
           
//             camera1 = true;
//             printf("Camera 1: ON\n");
//             hasTriggered=1;

//             // Alert the caller of this task to trigger the camera
//             if (camera_trigger_callback) {
//                 camera_trigger_callback();
//             }

//             // Delete this task when it's done
//             //vTaskDelete(NULL);
//             //break;
            
//         }



//         //  if(distance3  * 100 > THRESHOLD_DISTANCE){
//         //      if(camera2 == true){

//         //     printf("Car ENTERING zone 2\n");

//         //   }
//         //     camera2 = false;

//         // }

//         // if(distance4 * 100 > THRESHOLD_DISTANCE){
//         //      if(camera2 == true){

//         //     printf("Car LEAVING zone 2\n");

//         //   }
//         //     camera2 = false;
//         // }

//         // if(distance3 * 100 <THRESHOLD_DISTANCE && distance4 * 100 < THRESHOLD_DISTANCE && camera2 == false){
//         //     camera2 = true;
//         //     printf("Camera 2: ON\n");
//         // }


//         // if(distance5  * 100 > THRESHOLD_DISTANCE){
//         //      if(camera3 == true){

//         //     printf("Car ENTERING zone 3\n");

//         //   }
//         //     camera3 = false;

//         // }

//         // if(distance6 * 100 > THRESHOLD_DISTANCE){
//         //      if(camera3 == true){

//         //     printf("Car LEAVING zone 3\n");

//         //   }
//         //     camera3 = false;
//         // }

//         // if(distance5 * 100 <THRESHOLD_DISTANCE && distance6 * 100 < THRESHOLD_DISTANCE && camera3 == false){
//         //     camera3 = true;
//         //     printf("Camera 3: ON\n");
//         // }

//         // if(distance7  * 100 > THRESHOLD_DISTANCE){
//         //      if(camera4 == true){

//         //     printf("Car ENTERING zone 4\n");

//         //   }
//         //     camera4 = false;

//         // }

//         // if(distance8 * 100 > THRESHOLD_DISTANCE){
//         //      if(camera4 == true){

//         //     printf("Car LEAVING zone 4\n");

//         //   }
//         //     camera4 = false;
//         // }

//         // if(distance7 * 100 <THRESHOLD_DISTANCE && distance8 * 100 < THRESHOLD_DISTANCE && camera4 == false){
//         //     camera4 = true;
//         //     printf("Camera 4: ON\n");
//         // }




//         vTaskDelay(pdMS_TO_TICKS(70));
//     }
// }

// void app_main()
// {
//     xTaskCreate(ultrasonic_test, "ultrasonic_test", configMINIMAL_STACK_SIZE * 3, NULL, 5, NULL);
// }

// uint8_t getTriggered(){
//   return hasTriggered;
// }