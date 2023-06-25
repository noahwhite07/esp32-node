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

static const char *TAG = "main";
int xbee_cmd_callback(const xbee_cmd_response_t FAR *response);
void node_discovered( xbee_dev_t *xbee, const xbee_node_id_t *rec);

int gotResponse;
int discoveredAddress;

// I don't understand what this does yet
const xbee_dispatch_table_entry_t xbee_frame_handlers[] =
{
   XBEE_FRAME_HANDLE_LOCAL_AT,
   XBEE_FRAME_MODEM_STATUS_DEBUG,
   XBEE_FRAME_HANDLE_REMOTE_AT,
   XBEE_FRAME_HANDLE_ATND_RESPONSE,
   XBEE_FRAME_HANDLE_AO0_NODEID,
   

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

    // Initialize the AT Command layer for this XBee device
    xbee_cmd_init_device(&my_xbee);
    vTaskDelay(pdMS_TO_TICKS(1000));
    ESP_LOGI(TAG, "Waiting for XBee device to initialize...\n");
    while (xbee_cmd_query_status(&my_xbee) == -EBUSY) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    ESP_LOGI(TAG, "XBee initialized\n");

   // Register a callback function for node discovery
    xbee_disc_add_node_id_handler(&my_xbee, &node_discovered);

    // Send the command to start discovery
    xbee_disc_discover_nodes( &my_xbee, NULL);

    int status;

    // Wait for a response in a non-blocking fashion
    do {
        status = xbee_dev_tick(&my_xbee);
        if (status < 0) {
            ESP_LOGI(TAG, "Error %d from xbee_dev_tick().\n", status);      
        }             
        
    } while (status == -EBUSY || gotResponse == 0);

}




// A callback for when a node is discovered

// TODO: Figure out why this sometimes gets called with a null pointer to xbee_node_id_t
void node_discovered( xbee_dev_t *xbee, const xbee_node_id_t *rec){
    if(!rec){
        ESP_LOGI(TAG, "oopsie!\n");

    }else{
        char* nodeInfo = rec->node_info;
        discoveredAddress = rec->device_type;
        ESP_LOGI(TAG, "Node Discovered: %s\n", nodeInfo);
        gotResponse = 1;
    }
}