#include <stdio.h>
#include "my_component.h"
#include "esp_log.h"

static const char* TAG = "test";

void func(void)
{

}

void printxxx(char* str){
    ESP_LOGI(TAG, "%sxxx", str);
}