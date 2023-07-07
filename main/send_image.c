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

#define CHUNK_SIZE 45

static const char *TAG = "send_image";
void sendFrame(void * pvParameters);
void sendImage(void *pvParameters);

// Callback for when image has completed downloading from web server
void on_image_downloaded(uint8_t* img_buff, size_t img_size );

typedef struct {
    uint8_t* image;
    size_t image_size;
}send_image_params_t;

uint16_t chunks_sent;

//uint8_t * image_buff;

// Indicates that the xbee is ready to recieve another frame
uint8_t ready_to_send_frame = 1;

//size_t fetched_image_size;


void app_main(void)
{
 
    // Initialize communication with the xbee
    xbee_init();
    

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

    ESP_LOGI(TAG, "Wifi initialized");


    //=========================================================================//
    //    Fetch image from camera
    //=========================================================================//
    
    // Create a params struct to pass into the download image function
    download_image_params_t * download_img_params = malloc(sizeof(download_image_params_t));
    download_img_params->callbackFunction = on_image_downloaded;


    // Pass the camera 1 struct into the download_image params
    download_img_params->cam_zone_num = 1;

    ESP_LOGI(TAG, "Downloading image");

    // Begin downloading the image
    xTaskCreate(download_image, "download_image", 8192, download_img_params, 5, NULL);


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

void on_image_downloaded(uint8_t* img_buff, size_t img_size){

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
    free(img_buff);

    // Save a reference to the new image buffer and record its size
    //image_buff = new_img_buff;
    //fetched_image_size = img_size;



    ESP_LOGI(TAG, "Image size: %zu", img_size);

    // Create a params struct to pass into the send image function
    send_image_params_t * send_image_params = malloc(sizeof(send_image_params_t));
    send_image_params->image = new_img_buff;
    send_image_params->image_size = img_size;
    
    // Create the task
    xTaskCreate(sendImage, "SendImageTask", 8192, send_image_params, 2, NULL);
}



void sendImage(void *pvParameters){
    send_image_params_t *params = (send_image_params_t *)pvParameters;

    uint8_t* image = params->image;
    size_t image_size = params->image_size;
    
    vTaskDelay(pdMS_TO_TICKS(100));

    // Start measuring the time required to transmit the image
    int64_t start_time = esp_timer_get_time();

    chunks_sent = 0;

    // Send each chunk to the remote xbee
    ESP_LOGI(TAG, "Fetched image size: %zu", image_size);
    for (size_t i = 0; i < image_size; i += CHUNK_SIZE) {
        size_t chunk_size = (i + CHUNK_SIZE <= image_size) ? CHUNK_SIZE : image_size - i;
        

        // We should only try to call this again if the previous chunk was sent sucesfully
        // So the sendChunk function should return a success value

        
        //sendChunk(&img[i], chunk_size);

        send_frame_params_t send_frame_params = {
            .size = chunk_size,
            .payload = &image[i]
        };

        //ESP_LOGI(TAG, "Called sendFrame");

        // Create a task for sending the frame with priority 2
        //xTaskCreate(sendFrame, "Send Frame", 4096, &send_frame_params, 2, NULL);
        
        sendFrame(&send_frame_params); 

        // Image fetched from cam
        //sendChunk(&image_buff[i], chunk_size);

        // Free the memory allocated to the image after it's been sent
        //free(image_buff); // TODO: why does this throw an error?
        

    }

    send_frame_params_t send_frame_params = {
        .size = 4,
        .payload = (uint8_t *)"done"
    };


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



