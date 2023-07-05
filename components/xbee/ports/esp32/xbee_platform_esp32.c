#include "xbee/platform.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

uint32_t xbee_seconds_timer(void) {
    // Get the current tick count and convert it to seconds
    return xTaskGetTickCount() * portTICK_PERIOD_MS / 1000;
}

uint32_t xbee_millisecond_timer(void) {
    // Get the current tick count and convert it to milliseconds
    return xTaskGetTickCount() * portTICK_PERIOD_MS;
}