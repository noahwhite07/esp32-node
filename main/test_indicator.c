#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_idf_version.h>
// #include <max7219.h>
#include "indicator.h"

// #ifndef APP_CPU_NUM
// #define APP_CPU_NUM PRO_CPU_NUM
// #endif

//should work for all esp IDF versions with this
// #if ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(4, 0, 0)
// #define HOST    HSPI_HOST
// #else
// #define HOST    SPI2_HOST
// #endif

// #define CASCADE_SIZE 1

// #define MOSI_PIN 23
// #define CS_PIN 5
// #define CLK_PIN 18

// // Arrow symbols index in the symbols array
// #define RIGHT_ARROW 0
// #define UP_ARROW 1
// #define LEFT_ARROW 2
// #define X_ARROW 3


// // **** Orientation for display is main / master connections pins on the right side ****
// static const uint64_t symbols[] = {
//     0x10307efe7e301000, // arrow right
//     0x383838fe7c381000, // arrow up
//     0x1018fcfefc181000,  // arrow left
//     0x8142241818244281, // X symbol (No space available)

// };

// //test symbols array size
// //static const size_t symbols_size = sizeof(symbols) - sizeof(uint64_t) * CASCADE_SIZE;

// static max7219_t dev;
// static size_t currentArrow = 0;

/*
    destination_zone indicates the zone to be pointed to by this indicator.
    
    In the case of this particular node, zone 1 is to the right, zone 2 to the left,
    zone 3 is the ramp and all spots above floor 0, and zone 4 represents the area 
    outside of the garage. 

    This function should update the state of the indicator attatched to this node 
    such that it displays an arrow pointing to the correct zone, or 'X' if the garage is 
    full, in which case `destination_zone` will be zone 4
*/ 
// void updateIndicator(uint8_t destination_zone)
// {
//     switch (destination_zone)
//     {
//     case 1:
//         currentArrow = RIGHT_ARROW; 
//         break;
//     case 2:
//         currentArrow = LEFT_ARROW;
//         break;
//     case 3:
//         currentArrow = UP_ARROW;
//         break;
//     case 4:
//         currentArrow = X_ARROW;
//         break;
//     default:
//         return;
//     }

//     //draw to the display the currentArrow status
//     max7219_draw_image_8x8(&dev, 0, (uint8_t *)symbols + currentArrow * sizeof(uint64_t));
// }

void task(void *pvParameter)
{
    // //configure SPI bus for the MAX7219 driver 
    // spi_bus_config_t cfg = {
    //     .mosi_io_num = MOSI_PIN,
    //     .miso_io_num = -1,
    //     .sclk_io_num = CLK_PIN,
    //     .quadwp_io_num = -1,
    //     .quadhd_io_num = -1,
    //     .max_transfer_sz = 0,
    //     .flags = 0
    // };
    // //initialize the SPI bus
    //  ESP_ERROR_CHECK(spi_bus_initialize(HOST, &cfg, 1));

    // //configure the MAX7219 driver
    // dev.cascade_size = CASCADE_SIZE;
    // dev.digits = 0;
    // dev.mirrored = true;
    

    // //initialize the MAX7219 driver
    //  ESP_ERROR_CHECK(max7219_init_desc(&dev, HOST, MAX7219_MAX_CLOCK_SPEED_HZ, CS_PIN));
    //  ESP_ERROR_CHECK(max7219_init(&dev));

    while (1)
    {
        //need a delay to see changes (without it causes task watchdog timeout)
        vTaskDelay(pdMS_TO_TICKS(1000));  // Delay for 1 second (adjust as needed)

        // Example usage to update the arrow display
        updateIndicator(3);  // Update to display the indicator
    }
}

void app_main()
{
    init_indicator();
    xTaskCreatePinnedToCore(task, "task", configMINIMAL_STACK_SIZE * 3, NULL, 5, NULL, APP_CPU_NUM);
}