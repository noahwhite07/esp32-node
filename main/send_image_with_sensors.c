#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "xbee/platform.h"
#include "xbee/device.h"
#include "xbee/atcmd.h"
#include "esp_log.h"
#include "stdint.h"
#include "xbee/atcmd.h"  
#include "wpan/types.h"
#include "xbee/discovery.h"
#include "xbee/byteorder.h"
#include "xbee/wpan.h" 
#include "esp_timer.h"

#include "output.h"
#include "fetch_image.h"
#include "node_xbee.h"
#include "ultrasonic_sensors.h"
#include "esp32_pin_aliases.h"
#include "node_sensor_config.h"

#define CHUNK_SIZE 45

static const char *TAG = "send_image";
void sendFrame(void * pvParameters);
void sendImage(void *pvParameters);
void on_camera_triggered(uint8_t zone_number);
void init_sensors();

// Callback for when image has completed downloading from web server
void on_image_downloaded(uint8_t* img_buff, size_t img_size, uint8_t zone);

void on_frame_receieved(uint8_t indicator_state);

typedef struct {
    uint8_t* image;
    size_t image_size;
    uint8_t zone;
}send_image_params_t;

uint16_t chunks_sent;

// Indicates that the xbee is ready to recieve another frame
uint8_t ready_to_send_frame = 1;


void app_main(void)
{
    // Initialize communication with the xbee
    xbee_init(on_frame_receieved);

    // Set the RTS pin to low (this should be done in the xbee_init function)
    gpio_set_direction(GPIO_NUM_4, GPIO_MODE_OUTPUT);
    gpio_set_level(GPIO_NUM_4, 0);

    //=========================================================================//
    //    Init WiFi connection to ESP32-CAMs
    //=========================================================================//

    ESP_LOGI(TAG, "Initializing connection to cameras");

    // TODO: This should actually be a blocking function
    xTaskCreate(init_cameras, "init_cameras", 4096, NULL, 2, NULL);

    // Wait for the wifi connection to initialize
    while(!cameras_initialized()){
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    ESP_LOGI(TAG, "Cameras initialized");

    //=========================================================================//
    //    Start polling sensors in the background
    //=========================================================================//

    ESP_LOGI(TAG, "Initializing sensors");
    init_sensors();
    ESP_LOGI(TAG, "Sensors initialized");


    //=========================================================================//
    //    Print diagnostic information
    //=========================================================================//

    size_t buffered_size;
    esp_err_t ret = uart_get_buffered_data_len(UART_NUM_2, &buffered_size);
    if (ret == ESP_OK) {
        printf("Number of bytes in the UART2 TX buffer: %d\n", buffered_size);
    } else {
        printf("Error getting the buffered data length\n");
    }
  
}

void on_frame_receieved(uint8_t indicator_state){
    ESP_LOGI(TAG, "Frame recieved. New state: %d", indicator_state);

}

/*
    A callback function for when the camera has triggered

    Takes in the number of the zone that was crossed into
*/ 

void on_camera_triggered(uint8_t zone_number){
    ESP_LOGI(TAG, "Zone %d entered", zone_number);

    // Create a params struct to pass into the download image function
    download_image_params_t * download_img_params = malloc(sizeof(download_image_params_t));
    download_img_params->callbackFunction = on_image_downloaded;

    // Pass the camera 1 struct into the download_image params
    download_img_params->cam_zone_num = zone_number;

    ESP_LOGI(TAG, "Downloading image");

    // Begin downloading the image
    xTaskCreate(download_image, "download_image", 8192, download_img_params, 4, NULL);
}


void on_image_downloaded(uint8_t* img_buff, size_t img_size, uint8_t zone_num){

    // Allocate a new buffer to store a copy of the image
    uint8_t* new_img_buff = (uint8_t*) malloc(img_size);
    
    // Check if allocation succeeded
    if (new_img_buff == NULL) {
        ESP_LOGE(TAG, "Memory allocation failed for image buffer");
        return;
    }
    
    // Copy the content of the original buffer to the new buffer
    memcpy(new_img_buff, img_buff, img_size);

    // Free the original buffer
    //free(img_buff); //this is implicitly freed when it is realloced in the HTTP handler


    ESP_LOGI(TAG, "Image size: %zu", img_size);

    // Create a params struct to pass into the send image function
    send_image_params_t * send_image_params = malloc(sizeof(send_image_params_t));
    send_image_params->image = new_img_buff;
    send_image_params->image_size = img_size;
    send_image_params->zone = zone_num; // TODO; Implement actual logic for this
    
    // Create the task
    xTaskCreate(sendImage, "SendImageTask", 8192, send_image_params, 3, NULL);
}


void sendImage(void *pvParameters){
    send_image_params_t *params = (send_image_params_t *)pvParameters;

    uint8_t* image = params->image;
    size_t image_size = params->image_size;
    uint8_t zone = params->zone;
    
    vTaskDelay(pdMS_TO_TICKS(100));

    // Start measuring the time required to transmit the image
    int64_t start_time = esp_timer_get_time();

    chunks_sent = 0;

    // Send each chunk to the remote xbee
    ESP_LOGI(TAG, "Fetched image size: %zu", image_size);
    for (size_t i = 0; i < image_size; i += CHUNK_SIZE) {
        size_t chunk_size = (i + CHUNK_SIZE <= image_size) ? CHUNK_SIZE : image_size - i;


        send_frame_params_t send_frame_params = {
            .size = chunk_size,
            .payload = &image[i]
        };
        
        sendFrame(&send_frame_params); 
    }
    
    
    // Create a buffer to hold the payload string
    char payload_buffer[10]; 

    // Format the string to be "zone X", where X is the value of zone
    sprintf(payload_buffer, "zone %d", zone);

    // Send a sequence to the remote xbee to signal the end of an image transmission
    send_frame_params_t send_frame_params = {
        .size = strlen(payload_buffer), // +1 for the null character at the end of the string
        .payload = (uint8_t *)payload_buffer
    };

    // Send a sequence to the remote xbee to signal the end of an image transmission
    // send_frame_params_t send_frame_params = {
    //     .size = 6,
    //     .payload = (uint8_t *)"zone 1"
    // };
    sendFrame(&send_frame_params);

    

    // Fetch the time at which the image finished sending
    int64_t end_time = esp_timer_get_time();

    // Calculate and log the elapsed time in seconds
    double elapsed_time = (double)(end_time - start_time) / 1000000.0;

    ESP_LOGI(TAG, "Elapsed time: %.2f seconds", elapsed_time);
    ESP_LOGI(TAG, "Chunks sent: %d.", chunks_sent);
    ESP_LOGI(TAG, "Done. Exiting...\n");

    // Free the memory allocated to the image once it is done being sent
    free(image); 

    free(pvParameters);

    // Clear the UART TX buffer after transmission 
    //xbee_ser_tx_flush(&my_xbee);

    vTaskDelay(1000);

    // Delete this task when its done
    vTaskDelete(NULL);
}

void init_sensors(){
    //========================================================//
    // Start polling sensor 1
    //========================================================//

    sensor_pair_t *s1 = (sensor_pair_t *)malloc(sizeof(sensor_pair_t));
    s1->trig =  S1_TRIG;
    s1->echo_a = S1_ECHO_A;
    s1->echo_b = S1_ECHO_B;
    s1->zone = 1;

    register_threshold_params_t *s1_params = (register_threshold_params_t *)malloc(sizeof(register_threshold_params_t));
    s1_params->sensor_pair = *s1;
    s1_params->callback_function = on_camera_triggered;

    xTaskCreate(register_threshold, "register_threshold", 2048, s1_params, 5, NULL);

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

    xTaskCreate(register_threshold, "register_threshold", 2048, s2_params, 5, NULL);

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
        
    xTaskCreate(register_threshold, "register_threshold", 2048, s3_params, 5, NULL);
}



