#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
// #include "esp_system.h"
#include "xbee/platform.h"
#include "xbee/device.h"
#include "xbee/atcmd.h"
#include "esp_log.h"
#include "stdint.h"
#include "xbee/atcmd.h"  
#include "wpan/types.h"

static const char *TAG = "example";
int xbee_cmd_callback(const xbee_cmd_response_t FAR *response);

int chResponseNumber;
int gotResponse;

// I don't understand what this does yet
const xbee_dispatch_table_entry_t xbee_frame_handlers[] =
{
   XBEE_FRAME_HANDLE_LOCAL_AT,
   XBEE_FRAME_MODEM_STATUS_DEBUG,
   XBEE_FRAME_HANDLE_REMOTE_AT,
   

   XBEE_FRAME_TABLE_END
};

void app_main(void)
{
    gotResponse = 0;

    // Initialize the xbee_serial_t struct with your UART settings
    xbee_serial_t XBEE_SERPORT = {
        .uart_num = 2,          // UART port number 2
        .baudrate = 115200,  // Baud rate 115200
        .cts_pin = 15,
        .rts_pin = 12,
        .rx_pin = 16,
        .tx_pin = 17
    };

    // Initialize the xbee_dev_t struct
    xbee_dev_t my_xbee;

    // Open the XBee device
    if (xbee_dev_init(&my_xbee, &XBEE_SERPORT, NULL, NULL) != 0) {
        ESP_LOGI(TAG, "Failed to initialize device.\n");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    // Initialize the AT command sending
    xbee_cmd_init_device(&my_xbee);
    vTaskDelay(pdMS_TO_TICKS(1000));
    ESP_LOGI(TAG, "Waiting for XBee device to initialize...\n");
    while (xbee_cmd_query_status(&my_xbee) == -EBUSY) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    ESP_LOGI(TAG, "XBee initialized\n");

    const char* param = "CE";

    // Create an AT command to send to the xbee
    int16_t cmdHandle = xbee_cmd_create(&my_xbee, param);
    if(cmdHandle < 0){
        ESP_LOGI(TAG, "Error creating AT command\n");
    }

    // Register the callback funciton to handle this command's response
    if(xbee_cmd_set_callback(cmdHandle, xbee_cmd_callback, NULL) != 0){
        ESP_LOGI(TAG, "Error setting callback function\n");
    }
    
    // Stores the 64-bit IEEE address of the coordinator module
    #define COORD_ADDR 0x0013A2004219170D

    // Instantiate an addr64 union to pass into the set_remote function
    addr64 remoteAddr = {.b = 
        {0x00, 0x13, 0xA2, 0x00, 0x42, 0x19, 0x17, 0x0D}
    };

    // Set the target of the command to the remote coordinator xbee
    //xbee_cmd_set_target(cmdHandle, &remoteAddr, WPAN_NET_ADDR_UNDEFINED);

    // Attempt to send the command to the Xbee device
    int cmdSent = xbee_cmd_send(cmdHandle);


    if(cmdSent != 0){
        ESP_LOGI(TAG, "Error sending AT command\n");
    }

    int status;

    // Wait for a response in a non-blocking fashion
    do {
        //linelen = xbee_readline(cmdstr, sizeof cmdstr);
        status = xbee_dev_tick(&my_xbee);
        if (status < 0) {
            ESP_LOGI(TAG, "Error %d from xbee_dev_tick().\n", status);      
        }
            //ESP_LOGI(TAG, "Status: %d.\n", status);      
        
    } while (status == -EBUSY || gotResponse == 0);

   
}

// A callback for handling an xbee at command response
int xbee_cmd_callback(const xbee_cmd_response_t FAR *response)
{
        gotResponse = 1;
        ESP_LOGI(TAG, "Here2\n");
        const uint8_t *responseBytes = response->value_bytes;
        uint_fast8_t length = response->value_length;

        ESP_LOGI(TAG, "Response: %d", *responseBytes);

        return XBEE_ATCMD_DONE;
}
