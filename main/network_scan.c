#include <stdio.h>


#include "xbee/platform.h" // These are all fine
#include "xbee/byteorder.h"
#include "xbee/device.h"
#include "xbee/atcmd.h"
#include "xbee/scan.h"
#include "wpan/types.h"
#include "esp_log.h"

static const char *TAG = "example";

void app_main(void){
    ESP_LOGI(TAG, "Hello from network_Scan.c");
}