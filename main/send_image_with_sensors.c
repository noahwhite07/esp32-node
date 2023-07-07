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

#include "output.h"
#include "fetch_image.h"
#include "ultrasonic_sensors.h"

#define UNUSED(x) (void)(x)

#define CHUNK_SIZE 45

static const char *TAG = "send_image";
void sendFrame(uint8_t * payload, uint16_t size);
void sendChunk(const uint8_t *chunk, size_t size);
void sendImage();
void sendImageTask(void *pvParameters);

// Header for a 0x11 TX frame
xbee_header_transmit_explicit_t header;

// Initialize the xbee_dev_t struct
xbee_dev_t my_xbee;

uint16_t chunks_sent;


uint8_t * image_buff;

// Frame handler table for routing incoming frames
const xbee_dispatch_table_entry_t xbee_frame_handlers[] =
{
    XBEE_FRAME_HANDLE_LOCAL_AT,
    XBEE_FRAME_MODEM_STATUS_DEBUG,
    XBEE_FRAME_HANDLE_ATND_RESPONSE,
    XBEE_FRAME_HANDLE_AO0_NODEID,

    XBEE_FRAME_TABLE_END
};

void app_main(void)
{

    //=========================================================================//
    //    Initialize XBee module
    //=========================================================================//

    // Initialize the xbee_serial_t struct with your UART settings
    xbee_serial_t XBEE_SERPORT = {
        .uart_num = 2,          // UART port number 2
        .baudrate = 115200,  // Baud rate 115200
        .cts_pin = 15,
        .rts_pin = 12,
        .rx_pin = 16,
        .tx_pin = 17
    };


    // Open the XBee device
    if (xbee_dev_init(&my_xbee, &XBEE_SERPORT, NULL, NULL) != 0) {
        ESP_LOGI(TAG, "Failed to initialize device.\n");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    // Initialize the AT Command layer for this XBee device
    xbee_cmd_init_device(&my_xbee);
    vTaskDelay(pdMS_TO_TICKS(1000));
    ESP_LOGI(TAG, "Waiting for XBee device to initialize...\n");
    while (xbee_cmd_query_status(&my_xbee) == -EBUSY) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    ESP_LOGI(TAG, "XBee initialized\n");

    // Keep xbee_dev functions from using flow control
    xbee_dev_flowcontrol(&my_xbee, 0);
    xbee_dev_dump_settings(&my_xbee, 0);

 
    //=========================================================================//
    //    Instantiate XBee frame header
    //=========================================================================//
    
    // Instantiate an addr64 union to pass into the set_remote function
    addr64 remoteAddr = {.b = 
        {0x00, 0x13, 0xA2, 0x00, 0x42, 0x0E, 0x74, 0x72}
    };


    // Fill in the frame header according to our network configuration
    header.frame_type = (uint8_t) XBEE_FRAME_TRANSMIT_EXPLICIT;
    //header.frame_id = (uint8_t) xbee_next_frame_id(&my_xbee);
    header.frame_id = 1;
    header.ieee_address = remoteAddr;
    header.network_address_be = htobe16(WPAN_NET_ADDR_UNDEFINED); 
    header.source_endpoint = 0xE8;
    header.dest_endpoint = 0xE8;
    header.cluster_id_be = htobe16(0x11);
    header.profile_id_be = htobe16(WPAN_PROFILE_DIGI); 
    header.broadcast_radius = 0;
    header.options = 0x0;

    
    //{0x11, 0x03, 0x00, 0x13, 0xa2, 0x00, 0x42, 0x0e, 0x74, 0x72, 0xfe, 0xff, 0xe8, 0xe8, 0x11, 0x00, 0x05, 0xc1, 0x00, 0x00}

    //sendFrame((uint8_t *)"hey", 3);
    //sendImage();

    //=========================================================================//
    //    Init WiFi connection to ESP32-CAM
    //=========================================================================//
    ESP_LOGI(TAG, "Initializing WiFi connection...");

    xTaskCreate(init_cameras, "init_cameras", 4096, NULL, 2, NULL);

    // Wait for the wifi connection to initialize
    while(!cameras_initialized()){
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    ESP_LOGI(TAG, "Wifi initialized");
    //=========================================================================//
    //   Poll ultrasonics
    //=========================================================================//
    ESP_LOGI(TAG, "Polling ultrasonics");

    xTaskCreate(ultrasonic_test, "ultrasonic_test", 4096, NULL, 5, NULL);    

    void ultrasonic_test(void *pvParameters);
    while(!getTriggered()){
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    //=========================================================================//
    //    Fetch image from camera
    //=========================================================================//

    // Begin downloading the image
    xTaskCreate(download_image, "download_image", 4096, NULL, 5, NULL);

    ESP_LOGI(TAG, "Downloading image");

    // Wait for the image to finish downloading
    while(!image_ready()){
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    ESP_LOGI(TAG, "Image downloaded");

    // Save a reference to the image and record its size
    image_buff = get_image_buffer();
    size_t imageSize = get_image_size();

    ESP_LOGI(TAG, "Image size: %zu", imageSize);

    //=========================================================================//
    //    Send image to remote XBee module
    //=========================================================================//

    // Stack size in words, not bytes
    const uint16_t stackSize = 4096;

    // Task priority (higher number means higher priority)
    const UBaseType_t taskPriority = 2;

    // Create the task
    xTaskCreate(sendImageTask, "SendImageTask", stackSize, NULL, taskPriority, NULL);


   
}



// Send an explicit TX frame (0x11) to the remote Xbee device with the given payload
void sendFrame(uint8_t * payload, uint16_t size){

    uint16_t datalen = size; 
    chunks_sent++;
    
    //ESP_LOGI(TAG, "Using payload of size %d\n", datalen);
    //header.frame_id = (uint8_t) xbee_next_frame_id(&my_xbee);

    // Send a 0x11 explicit TX frame to the remote Xbee device
    xbee_frame_write(&my_xbee, &header, sizeof(header), payload, datalen, 0); // xbee_frame_write: frame type 0x11, id 0x03 (24-byte payload)

}

void sendChunk(const uint8_t *chunk, size_t size) {
    vTaskDelay(pdMS_TO_TICKS(30));
    //taskYIELD();
    //printf("sending chunk of size %zu: {", size);
    //for (size_t i = 0; i < size; i++) {
        // printf("0x%02X", chunk[i]);
        // if (i < size - 1) {
        //     printf(", ");
        // }
    //}
    //printf("}\n");
    //taskYIELD();

    sendFrame(chunk, CHUNK_SIZE);
   
    
}

void sendImageTask(void *pvParameters){
    sendImage();

    // Delete this task when its done
    vTaskDelete(NULL);
}

void sendImage(){
    vTaskDelay(pdMS_TO_TICKS(200));
    chunks_sent = 0;
    // Use the first chunk of the sample image as an example
    //uint8_t chunk[] = { 0xFF, 0xD8, 0xFF, 0xE0, 0x00, 0x10, 0x4A, 0x46, 0x49, 0x46, 0x00, 0x01};

    // Send the chunk to the remote xbee
    //sendChunk(chunk, sizeof(chunk)/sizeof(chunk[0]));

    for (size_t i = 0; i < img_size; i += CHUNK_SIZE) {
        size_t chunk_size = (i + CHUNK_SIZE <= img_size) ? CHUNK_SIZE : img_size - i;
        
        // Test image (hardcoded)
        //sendChunk(&img[i], chunk_size);
        
        // Image fetched from cam
        sendChunk(&image_buff[i], chunk_size);

    }

    sendFrame((uint8_t *)"done", 4);
    ESP_LOGI(TAG, "Chunks sent: %d.", chunks_sent);
    ESP_LOGI(TAG, "Done. Exiting...\n");
    vTaskDelay(1000);
}




