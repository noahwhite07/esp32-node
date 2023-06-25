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

static const char *TAG = "main";
int xbee_cmd_callback(const xbee_cmd_response_t FAR *response);
void node_discovered( xbee_dev_t *xbee, const xbee_node_id_t *rec);
int handle_frame(xbee_dev_t *xbee, const void *frame,uint16_t length, void *context);
int handle_tx_frame(xbee_dev_t *xbee, const void *frame,uint16_t length, void *context);
int handle_tx_expl_frame(xbee_dev_t *xbee, const void *frame,uint16_t length, void *context);

int handle_remote_response(xbee_dev_t *xbee, const void *frame,uint16_t length, void *context);


int gotResponse;
int discoveredAddress;

// Handler for remote AT command
#define XBEE_FRAME_HANDLE_CUSTOM {XBEE_FRAME_REMOTE_AT_RESPONSE, 0, handle_frame, NULL} 

// Custom handler for 0x10 digimesh frames //XBEE_FRAME_TRANSMIT
#define XBEE_FRAME_HANDLE_TX_REQ {XBEE_FRAME_TRANSMIT, 0, handle_tx_frame, NULL} 

// Custom handler for 0x10 digimesh frames //XBEE_FRAME_TRANSMIT
#define XBEE_FRAME_HANDLE_TX_EXPL {XBEE_FRAME_TRANSMIT_EXPLICIT, 0, handle_tx_expl_frame, NULL} 

// Handler for transmit responses (0x90)
#define XBEE_FRAME_HANDLE_TX_EXPL {0x90, 0, handle_remote_response, NULL} 


const xbee_dispatch_table_entry_t xbee_frame_handlers[] =
{
   XBEE_FRAME_HANDLE_LOCAL_AT,
   XBEE_FRAME_MODEM_STATUS_DEBUG,
   XBEE_FRAME_HANDLE_TX_REQ,
   XBEE_FRAME_HANDLE_ATND_RESPONSE,
   XBEE_FRAME_HANDLE_AO0_NODEID,
   XBEE_FRAME_HANDLE_CUSTOM, // Should handle frames of type 0x10
   XBEE_FRAME_HANDLE_TX_EXPL, //0x11

   XBEE_FRAME_TABLE_END
};

// Define a table entry struct for handling transmit frames (0x10)

/*#define XBEE_FRAME_HANDLE_REMOTE_AT    \
   { XBEE_FRAME_REMOTE_AT_RESPONSE, 0, _xbee_cmd_handle_response, NULL }*/



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

    // Keep xbee_dev functions from using flow control
    xbee_dev_flowcontrol(&my_xbee, 0);
    xbee_dev_dump_settings(&my_xbee, 0);
    //=========================================================================
    const char* param = "CE";

    // Create an AT command to send to the xbee
    int16_t cmdHandle = xbee_cmd_create(&my_xbee, param);
    if(cmdHandle < 0){
        ESP_LOGI(TAG, "Error creating AT command\n");
    }

    //Register the callback funciton to handle this command's response
    // if(xbee_cmd_set_callback(cmdHandle, xbee_cmd_callback, NULL) != 0){
    //     ESP_LOGI(TAG, "Error setting callback function\n");
    // }
    

    // Instantiate an addr64 union to pass into the set_remote function
    addr64 remoteAddr = {.b = 
        {0x00, 0x13, 0xA2, 0x00, 0x42, 0x0E, 0x74, 0x72}
    };

    // Set the target of the command to the remote coordinator xbee
    xbee_cmd_set_target(cmdHandle, &remoteAddr, WPAN_NET_ADDR_UNDEFINED);

    // Attempt to send the command to the Xbee device
    int cmdSent = xbee_cmd_send(cmdHandle);


    if(cmdSent != 0){
        ESP_LOGI(TAG, "Error sending AT command\n");
    }
    //=========================================================================

    //int xbee_frame_write( xbee_dev_t *xbee, const void FAR *header, uint16_t headerlen, const void FAR *data, uint16_t datalen, uint16_t flags)

    // Header for a 0x11 TX frame
    xbee_header_transmit_explicit_t header;

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

    // Instantiate payload string and get its size
    const uint8_t payload[] = "hey";
    uint16_t datalen = sizeof(payload) / sizeof(payload[0]); // 14 + #chars ???
    
    ESP_LOGI(TAG, "Using payload of size %d\n", datalen);
    ESP_LOGI(TAG, "header length: %d\n", sizeof(header));
    
    uint8_t payload2[] = {0x11, 0x01, 0x00, 0x13, 0xA2, 0x00, 0x42, 0x0E, 0x74, 0x72, 0xFF, 0xFE, 0xE8, 0xE8, 0x00, 0x11, 0xC1, 0x05, 0x00, 0x00, 0x68, 0x65, 0x79, 0x18};
    //xbee_frame_write(&my_xbee, NULL, 0, payload2, 23, 0); // Total size - (3+1) = 27-4 = 23

    // Send a 0x11 explicit TX frame to the remote Xbee device
    xbee_frame_write(&my_xbee, &header, sizeof(header), payload, datalen - 1, 0); // xbee_frame_write: frame type 0x11, id 0x03 (24-byte payload)

    //{0x11, 0x03, 0x00, 0x13, 0xa2, 0x00, 0x42, 0x0e, 0x74, 0x72, 0xfe, 0xff, 0xe8, 0xe8, 0x11, 0x00, 0x05, 0xc1, 0x00, 0x00}

    int status;

    // Wait for a response in a non-blocking fashion
    do {
        status = xbee_dev_tick(&my_xbee);
        if (status < 0) {
            ESP_LOGI(TAG, "Error %d from xbee_dev_tick().\n", status);      
        }             
        
    } /*while (status == -EBUSY || gotResponse == 0)*/while(1);
    ESP_LOGI(TAG, "Done. Exiting...\n");
}

// Frame handler callbacks
//====================================================================================
int handle_remote_response(xbee_dev_t *xbee, const void *raw,uint16_t length, void *context){
    ESP_LOGI(TAG, "Floople noops fingle bingle.\n");
    ESP_LOGI(TAG, "length: %d\n", length);

    /* For some reason, the reported "length" is always 12 greater than the actual # of payload bytes in the frame*/
    size_t payload_length = length - 12 ; // Length of the payload
    
    // Allocate a string with a space for a null terminator 
    uint8_t* null_terminated_payload = malloc(payload_length + 1);
    
    const xbee_frame_receive_t  *frame = raw;
    const uint8_t* payload = frame->payload;

    memcpy(null_terminated_payload, payload, payload_length); // Copy payload to new array
    null_terminated_payload[payload_length] = '\0'; // Append null character

    gotResponse = 1;
    ESP_LOGI(TAG, "Handling TX frame...\n");
    ESP_LOGI(TAG, "Got payload: %s\n", null_terminated_payload);
    printf("Got payload: %s\n", null_terminated_payload);

    free(null_terminated_payload);

    return 0;
}



// A callback for when an explicit address TX Request (0x11) frame is recieved
int handle_tx_expl_frame(xbee_dev_t *xbee, const void *frame,uint16_t length, void *context){
    gotResponse = 1;
    ESP_LOGI(TAG, "Handling TX frame...\n");

    return 0;
}

// A callback for when a TX Request (0x10) frame is recieved
int handle_tx_frame(xbee_dev_t *xbee, const void *frame, uint16_t length, void *context){
    gotResponse = 1;
    ESP_LOGI(TAG, "Handling TX frame...\n");

    return 0;
}

// A callback for when a node is discovered
int handle_frame(xbee_dev_t *xbee, const void *frame,uint16_t length, void *context){
    gotResponse = 1;
    ESP_LOGI(TAG, "Handling frame...\n");

    return 0;
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
