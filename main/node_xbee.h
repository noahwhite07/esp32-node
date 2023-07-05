/*
    A collection of functions for interacting with the XBee module via the ESP32
*/

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


// TODO: Include logic for recieving frames too
void sendFrame(void * pvParameters);

// A struct for passing in parameters to the sendFrame function so that it can be instantiated as an XTask
typedef struct {
    uint8_t * payload; 
    uint16_t size;
}send_frame_params_t;

// Frame handler table for routing incoming frames
//extern xbee_dispatch_table_entry_t xbee_frame_handlers[];

// Reference to the XBee struct
extern xbee_dev_t my_xbee;

// Header for a 0x11 TX frame
extern xbee_header_transmit_explicit_t header;

// Initialize communication with the xbee
void xbee_init(void);