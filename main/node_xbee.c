#include "node_xbee.h"

static const char *TAG = "node_xbee";

int xbee_handle_expl_rx(xbee_dev_t * xbee, const void * raw, uint16_t length, void * context);
void poll_incoming_frames(void* pvParameters);
// Keep statistics for success rate of sending frames
uint16_t frames_sent = 0;
uint16_t frames_dropped = 0;

FunctionPointer frame_rx_callback = NULL;

// Reference to the xbee library object
xbee_dev_t my_xbee;

// The header for our explicit tx frames
xbee_header_transmit_explicit_t header;

// Initialize the xbee_serial_t struct with your UART settings
xbee_serial_t XBEE_SERPORT = {
    .uart_num = 2,          // UART port number 2
    .baudrate = 230400,  
    .cts_pin = 2,//15,
    .rts_pin = 4 ,//12,
    .rx_pin = 16,
    .tx_pin = 17
};

// Define a custom handler for explicit RX frames (0x91)
#define XBEE_FRAME_HANDLE_RX_EXPL { 0x91, 0, xbee_handle_expl_rx, NULL }

// Define a custom handler for explicit RX frames (0x90)
//#define XBEE_FRAME_HANDLE_RX { 0x90, 0, xbee_handle_rx, NULL }

// Tell the XBee library which incoming frame types we care about recieveing 
const xbee_dispatch_table_entry_t xbee_frame_handlers[] = {
    XBEE_FRAME_HANDLE_LOCAL_AT,
    XBEE_FRAME_MODEM_STATUS_DEBUG,
    XBEE_FRAME_HANDLE_ATND_RESPONSE,
    XBEE_FRAME_HANDLE_AO0_NODEID,

    //Custom
    XBEE_FRAME_HANDLE_RX_EXPL,
    //XBEE_FRAME_HANDLE_RX,

    XBEE_FRAME_TABLE_END
};

int xbee_handle_expl_rx(xbee_dev_t * xbee, const void * raw, uint16_t length, void * context){
    ESP_LOGI(TAG, "length: %d\n", length);

    // /* For some reason, the reported "length" is always 12 greater than the actual # of payload bytes in the frame*/
    // size_t payload_length = length - 12; // Length of the payload
    
    // // Allocate a string with a space for a null terminator 
    // uint8_t* null_terminated_payload = malloc(payload_length + 1);
    
    // const xbee_frame_receive_t  *frame = raw;
    // const uint8_t* payload = frame->payload;

    // memcpy(null_terminated_payload, payload, payload_length); // Copy payload to new array
    // null_terminated_payload[payload_length] = '\0'; // Append null character

    // ESP_LOGI(TAG, "Handling TX frame...\n");
    // ESP_LOGI(TAG, "Got payload: %s\n", null_terminated_payload);
    // printf("Got payload: %s\n", null_terminated_payload);

    // free(null_terminated_payload);
    // Cast raw to uint8_t pointer for ease of processing
    uint8_t* raw_bytes = (uint8_t*)raw;
    
    printf("Raw data in hex: ");
    for(int i = 0; i < length; ++i) {
        printf("0x%02X", raw_bytes[i]);
        if(i != length - 1) {  // Avoid comma after last element
            printf(", ");
        }
    }
    printf("\n");

    // If the payload is exactly 1 character
    if(length == 19){
        ESP_LOGI(TAG, "Num received: %c", raw_bytes[length-1]);
        frame_rx_callback(raw_bytes[length-1] - '0');
    }
    
    return 0;
}



// //TODO: move this into main to act as a callback
// // A callback for 0x91 frames recieved from the remote xbee
// int xbee_handle_expl_rx(xbee_dev_t * xbee, const void * raw, uint16_t length, void * context){
//     ESP_LOGI(TAG, "EXPL RX FRAME RECEIVED");

//     return 0;
// }

// A callback for 0x90 frames recieved from the remote xbee
// int xbee_handle_rx(xbee_dev_t * xbee, const void * raw, uint16_t length, void * context){
//     ESP_LOGI(TAG, "RX FRAME RECEIVED");

//     return 0;
// }

void poll_incoming_frames(void* pvParameters){
    int status;

    // Wait for a response in a non-blocking fashion
    do {
        taskYIELD();
        status = xbee_dev_tick(&my_xbee);
        vTaskDelay(pdMS_TO_TICKS(1000));
        if (status < 0) {
            ESP_LOGI(TAG, "Error %d from xbee_dev_tick().\n", status);      
        }             
        
    } while(1);
}

// Send an explicit TX frame (0x11) to the remote Xbee device with the given payload
void sendFrame(void *pvParameters){ // TODO: Implement retryCount as a parameter
    //ESP_LOGI(TAG, "Here");
    //vTaskDelay(20);

    // Cast params pointer to the correct type
    send_frame_params_t *params = (send_frame_params_t *)pvParameters;

    // Extract the parameter values from the params struct
    uint16_t datalen = params->size; 
    uint8_t * payload = params->payload;

    
    
    //ESP_LOGI(TAG, "Using payload of size %d\n", datalen);
    header.frame_id = (uint8_t) xbee_next_frame_id(&my_xbee);

    //taskYIELD();

    // // Send a 0x11 explicit TX frame to the remote Xbee device
    // int res = xbee_frame_write(&my_xbee, &header, sizeof(header), payload, datalen, 0); // xbee_frame_write: frame type 0x11, id 0x03 (24-byte payload)
    
    
    // // If the TX buffer is full or the XBee has de-asserted CTS, wait for the buffer to empty or for CTS to be re-asserted
    // if(res != 0){
    //     ESP_LOGI(TAG, "Could not send frame, retrying in 100 ms ...");
    //     vTaskDelay(pdMS_TO_TICKS(100));
    //     sendFrame(payload, size);
    // }
    //vTaskDelay(pdMS_TO_TICKS(10));

    // Attempt to send the frame, retrying after 100 ms if the frame couldnt be send
    while(xbee_frame_write(&my_xbee, &header, sizeof(header), payload, datalen, 0) != 0){
        ESP_LOGI(TAG, "Could not send frame, retrying in 100 ms ...");

        frames_dropped++;

        size_t buffered_size;
        esp_err_t ret = uart_get_buffered_data_len(UART_NUM_2, &buffered_size);
        if (ret == ESP_OK) {
            printf("Number of bytes in the UART2 TX buffer: %d\n", buffered_size);
        } else {
            printf("Error getting the buffered data length\n");
        }
        //taskYIELD();
        vTaskDelay(pdMS_TO_TICKS(200));

        
    }

    

    frames_sent++;

    //ESP_LOGI(TAG, "Here");
 
    //vTaskDelete(NULL);
}

void xbee_init(FunctionPointer fp){

    //=========================================================================//
    //    Initialize XBee module
    //=========================================================================//

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
    xbee_dev_flowcontrol(&my_xbee, 1);
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

    // create a global reference to the frame receieved callback
    frame_rx_callback = fp;

    // Start polling for incoming frames in the background
    xTaskCreate(poll_incoming_frames, "poll_incoming_frames", 4096, NULL, 1, NULL);
}
