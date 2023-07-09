#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_idf_version.h>

#include "indicator.h"

void task(void *pvParameter)
{

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