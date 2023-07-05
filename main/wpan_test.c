#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "xbee/platform.h"
#include "xbee/device.h"
#include "xbee/atcmd.h"
#include "xbee/wpan.h"
#include "esp_log.h"
#include "stdint.h"
#include "xbee/atcmd.h"  
#include "wpan/types.h"
#include "xbee/discovery.h"

static const char *TAG = "main";
int xbee_cmd_callback(const xbee_cmd_response_t FAR *response);
void node_discovered( xbee_dev_t *xbee, const xbee_node_id_t *rec);
int handle_frame(xbee_dev_t *xbee, const void *frame,uint16_t length, void *context);
int handle_tx_frame(xbee_dev_t *xbee, const void *frame,uint16_t length, void *context);
int handle_tx_expl_frame(xbee_dev_t *xbee, const void *frame,uint16_t length, void *context);
int xbee_discovery_cluster_handler2(const wpan_envelope_t * envelope, void * context);

int gotResponse;
int discoveredAddress;


// Handler for remote AT command
#define XBEE_FRAME_HANDLE_CUSTOM {XBEE_FRAME_REMOTE_AT_RESPONSE, 0, handle_frame, NULL} 

// Custom handler for 0x10 digimesh frames //XBEE_FRAME_TRANSMIT
#define XBEE_FRAME_HANDLE_TX_REQ {0x90, 0, handle_tx_frame, NULL} 

// Custom handler for 0x10 digimesh frames //XBEE_FRAME_TRANSMIT
#define XBEE_FRAME_HANDLE_TX_EXPL {XBEE_FRAME_TRANSMIT_EXPLICIT, 0, handle_tx_expl_frame, NULL} 

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

/// Format of XBee API frame type 0x90 (#XBEE_FRAME_RECEIVE);
/// received from XBee by host.
typedef XBEE_PACKED(xbee_frame_receive_t, {
   uint8_t        frame_type;          ///< XBEE_FRAME_RECEIVE (0x90)
   addr64         ieee_address;
   uint16_t       network_address_be;
   uint8_t        options;             ///< bitfield, see XBEE_RX_OPT_xxx macros
   uint8_t        payload[1];          ///< multi-byte payload
}) xbee_frame_receive_t;

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

    xbee_ser_flowcontrol(&XBEE_SERPORT, 0);

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
        {0x00, 0x13, 0xA2, 0x00, 0x42, 0x19, 0x17, 0x0D}
    };

    // Set the target of the command to the remote coordinator xbee
    xbee_cmd_set_target(cmdHandle, &remoteAddr, WPAN_NET_ADDR_UNDEFINED);

    // Attempt to send the command to the Xbee device
    int cmdSent = xbee_cmd_send(cmdHandle);


    if(cmdSent != 0){
        ESP_LOGI(TAG, "Error sending AT command\n");
    }

    // WPAN configuration
    //=========================================================================
    

//     // must be sorted by cluster ID
//     const wpan_cluster_table_entry_t digi_data_clusters[] =
//     {
//     // transparent serial goes here (cluster 0x0011)
//     { DIGI_CLUST_SERIAL, xbee_discovery_cluster_handler2, NULL,
//         WPAN_CLUST_FLAG_INOUT | WPAN_CLUST_FLAG_NOT_ZCL },

//     // handle join notifications (cluster 0x0095) when ATAO is not 0
//     XBEE_DISC_DIGI_DATA_CLUSTER_ENTRY,

//     WPAN_CLUST_ENTRY_LIST_END
//     };
//     const wpan_endpoint_table_entry_t endpoints_table[] = {
// 	/* Add your endpoints here */
	

// 	/* Digi endpoints.  If you are not doing anything fancy, this is
//       boilerplate which allows discovery responses to flow through
//       to the correct handler.  */
// 	{
// 	   WPAN_ENDPOINT_DIGI_DATA,  // endpoint
// 	   WPAN_PROFILE_DIGI,        // profile ID
// 	   NULL,                     // endpoint handler
// 	   NULL,                     // ep_state
// 	   0x0000,                   // device ID
// 	   0x00,                     // version
// 	   digi_data_clusters        // clusters
// 	},

// 	{ WPAN_ENDPOINT_END_OF_LIST }
// };



// Attempt to initialize the WPAN API layer
// if (xbee_wpan_init(&my_xbee, endpoints_table) != 0) {
//         ESP_LOGI(TAG, "Failed to initialize device.\n");
//         vTaskDelay(pdMS_TO_TICKS(1000));
// }



//==============================================================================


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

// A callback for when an explicit address TX Request (0x11) frame is recieved
int handle_tx_expl_frame(xbee_dev_t *xbee, const void *raw, uint16_t length, void *context){
    gotResponse = 1;
    ESP_LOGI(TAG, "Handling explicit TX frame...\n");

    return 0;
}

// A callback for when a TX Request (0x10) frame is recieved
int handle_tx_frame(xbee_dev_t *xbee, const void *raw,uint16_t length, void *context){
    printf("Length: %d\n", length);

    
    size_t payload_length = length - 12 ; // Length of the payload

    //uint8_t* null_terminated_payload = malloc(payload_length + 1);
    
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

// A callback for when a node is discovered
int handle_frame(xbee_dev_t *xbee, const void *frame,uint16_t length, void *context){
    gotResponse = 1;
    ESP_LOGI(TAG, "Handling fwame...\n");

    return 0;
}

// A callback for handling an xbee at command response
int xbee_cmd_callback(const xbee_cmd_response_t FAR *response)
{
        gotResponse = 1;
        ESP_LOGI(TAG, "Here2\n");
        const uint8_t *responseBytes = response->value_bytes;

        ESP_LOGI(TAG, "Response: %d", *responseBytes);

        return XBEE_ATCMD_DONE;
}

// int xbee_discovery_cluster_handler2(const wpan_envelope_t * envelope, void * context){
//         ESP_LOGI(TAG, "cluster handler called here\n");
//         return 0;
// }

