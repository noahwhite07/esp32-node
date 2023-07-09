/*
    This file contains the logic to control the LED display attatched to the node
*/

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_idf_version.h>
#include "max7219.h"

//should work for all esp IDF versions with this
#if ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(4, 0, 0)
#define HOST    HSPI_HOST
#else
#define HOST    SPI2_HOST
#endif

#define CASCADE_SIZE 1

// GPIO pin connections between the indicator and node board
#define MOSI_PIN 19
#define CS_PIN 5
#define CLK_PIN 18

// Arrow symbols index in the symbols array
#define RIGHT_ARROW 0
#define UP_ARROW 1
#define LEFT_ARROW 2
#define X_ARROW 3


// **** Orientation for display is main / master connections pins on the right side ****
static const uint64_t symbols[] = {
    0x10307efe7e301000, // arrow right
    0x383838fe7c381000, // arrow up
    0x1018fcfefc181000,  // arrow left
    0x8142241818244281, // X symbol 

};

static max7219_t dev;
static size_t currentArrow = 0;

/*
    Initialize the SPI bus for controlling the LED matrix and
    set the graphic to the default state
*/
void init_indicator();


/*
    This function should update the state of the indicator attatched to this node 
    such that it displays an arrow pointing to the correct zone, or 'X' if the garage is 
    full, in which case `destination_zone` will be zone 4
    
    In the case of this particular node, zone 1 is to the right, zone 2 to the left,
    zone 3 is the ramp and all spots above floor 0, and zone 4 represents the area 
    outside of the garage. 
*/ 
void updateIndicator(uint8_t destination_zone);