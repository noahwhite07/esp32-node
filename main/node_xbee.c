#include "node_xbee.h"

static const char *TAG = "node_xbee";

// Keep statistics for success rate of sending frames
uint16_t frames_sent = 0;
uint16_t frames_dropped = 0;

// Reference to the xbee library object
xbee_dev_t my_xbee;

// The header for our explicit tx frames
xbee_header_transmit_explicit_t header;

// Initialize the xbee_serial_t struct with your UART settings
xbee_serial_t XBEE_SERPORT = {
    .uart_num = 2,          // UART port number 2
    .baudrate = 115200,  // Baud rate 115200
    .cts_pin = 15,
    .rts_pin = 12,
    .rx_pin = 16,
    .tx_pin = 17
};

// Tell the XBee library which incoming frame types we care about recieveing 
const xbee_dispatch_table_entry_t xbee_frame_handlers[] = {
    XBEE_FRAME_HANDLE_LOCAL_AT,
    XBEE_FRAME_MODEM_STATUS_DEBUG,
    XBEE_FRAME_HANDLE_ATND_RESPONSE,
    XBEE_FRAME_HANDLE_AO0_NODEID,
    XBEE_FRAME_TABLE_END
};

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
    vTaskDelay(pdMS_TO_TICKS(10));

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

void xbee_init(void){

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
}
