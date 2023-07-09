#include "indicator.h"

static const char *TAG = "indicator";


void init_indicator(){

     //configure SPI bus for the MAX7219 driver 
    spi_bus_config_t cfg = {
        .mosi_io_num = MOSI_PIN,
        .miso_io_num = -1,
        .sclk_io_num = CLK_PIN,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 0,
        .flags = 0
    };
    //initialize the SPI bus
     ESP_ERROR_CHECK(spi_bus_initialize(HOST, &cfg, 1));

    //configure the MAX7219 driver
    dev.cascade_size = CASCADE_SIZE;
    dev.digits = 0;
    dev.mirrored = true;
    

    //initialize the MAX7219 driver
    ESP_ERROR_CHECK(max7219_init_desc(&dev, HOST, MAX7219_MAX_CLOCK_SPEED_HZ, CS_PIN));
    ESP_ERROR_CHECK(max7219_init(&dev));
}

// TODO: ask aymen if its necessary to constantly call update
void updateIndicator(uint8_t destination_zone){
    switch (destination_zone)
    {
    case 1:
        currentArrow = RIGHT_ARROW; 
        break;
    case 2:
        currentArrow = LEFT_ARROW;
        break;
    case 3:
        currentArrow = UP_ARROW;
        break;
    case 4:
        currentArrow = X_ARROW;
        break;
    default:
        return;
    }

    //draw to the display the currentArrow status
    max7219_draw_image_8x8(&dev, 0, (uint8_t *)symbols + currentArrow * sizeof(uint64_t));
}