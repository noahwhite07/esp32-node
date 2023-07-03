#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_http_client.h"
#include "esp_log.h"

#include "fetch_image.h"

#define WIFI_SSID       "esp32-cam2"
#define WIFI_PASSWORD   "password"
#define SERVER_URL      "http://192.168.4.1/capture"

void init_wifi(void);


esp_netif_t *wifi_netif = NULL;

static const char *TAG = "main";

static uint8_t *image_buffer = NULL; // init wifi, wait for wifi initialized, download image, wait for image ready, send image
static size_t image_size = 0;

static uint8_t wifi_is_initialized = 0;
static uint8_t image_is_ready = 0;

esp_err_t http_event_handler(esp_http_client_event_t *evt)
{

    switch (evt->event_id) {
        case HTTP_EVENT_ERROR:
            ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
            break;
        case HTTP_EVENT_HEADER_SENT:
            ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
            break;
        case HTTP_EVENT_ON_HEADER:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER");
            break;
        case HTTP_EVENT_ON_DATA:
            ESP_LOGI(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
            

            if (evt->data == NULL) {
                    ESP_LOGE(TAG, "Data is NULL");
                    break;
            }

            // Reallocate the buffer to hold the new data
            image_buffer = realloc(image_buffer, image_size + evt->data_len);

            if (image_buffer == NULL) {
                ESP_LOGE(TAG, "Failed to allocate memory for output buffer");
                break;
            }

            // Ensure that the buffer does not overflow
            // if ((output_len + evt->data_len) > OUTPUT_BUFFER_SIZE) { // assuming OUTPUT_BUFFER_SIZE is the size of output_buffer
            //     ESP_LOGE(TAG, "Buffer overflow detected");
            //     break;
            // }

            // Copy the response into the buffer
            if (!esp_http_client_is_chunked_response(evt->client)) {
                //ESP_LOGI(TAG, "!chunked");
                memcpy(image_buffer + image_size, evt->data, evt->data_len);
                image_size += evt->data_len;
            }

            ESP_LOGI(TAG, "new image size: %d", image_size);

            break;
        case HTTP_EVENT_ON_FINISH:
            image_is_ready = 1;
            ESP_LOGI(TAG, "HTTP_EVENT_ON_FINISH");
            break;
        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGD(TAG, "HTTP_EVENT_DISCONNECTED");
            break;
        case HTTP_EVENT_REDIRECT:
            ESP_LOGD(TAG, "HTTP_EVENT_REDIRECT");
            break;
    }
    
    return ESP_OK;
}




void download_image(void *params)
{   
    // Extract the passed in callback function from the params struct
    download_image_params_t* download_params = (download_image_params_t*) params;


    ESP_LOGI(TAG, "Starting download task");
    image_is_ready = 0;

    // Configure the HTTP client
    esp_http_client_config_t config = {
        .url = SERVER_URL,
        .event_handler = http_event_handler,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);

    // Make the HTTP GET request
    esp_err_t err = esp_http_client_perform(client);
    
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Image downloaded successfully");

        
    } else {
        ESP_LOGE(TAG, "HTTP GET request failed: %s", esp_err_to_name(err));
    }

    
    

    // Alert the passed in callback function that the image is done downloading
    if (download_params && download_params->callbackFunction) {
        ESP_LOGI(TAG, "calling callback function");
        download_params->callbackFunction();
    }else{
        ESP_LOGI(TAG, "oopsie! teehee! :)");
    }

    esp_http_client_cleanup(client);
    vTaskDelete(NULL);
}

void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGI(TAG, "Retrying connection...");
        esp_wifi_connect();
    } else if (event_id == WIFI_EVENT_STA_CONNECTED) {
        ESP_LOGI(TAG, "Connected to WiFi");

        vTaskDelay(2000 / portTICK_PERIOD_MS);

        // Get the default station interface
        //esp_netif_t *netif = esp_netif_get_handle_from_type(ESP_IF_WIFI_STA);
        esp_netif_ip_info_t ip_info;
        if (esp_netif_get_ip_info(wifi_netif, &ip_info) == ESP_OK) {
            ESP_LOGI(TAG, "IP Address: " IPSTR, IP2STR(&ip_info.ip));
            ESP_LOGI(TAG, "Subnet Mask: " IPSTR, IP2STR(&ip_info.netmask));
            ESP_LOGI(TAG, "Gateway: " IPSTR, IP2STR(&ip_info.gw));
        } else {
            ESP_LOGI(TAG, "Could not get IP info");
        }
        
        wifi_is_initialized = 1;

         // Create task to download image after connected to WiFi
        //xTaskCreate(download_image, "download_image", 4096, NULL, 5, NULL);
        
    }
}

void init_wifi()
{
    wifi_is_initialized = 0;

    ESP_LOGI(TAG, "Initializing...");

    // Initialize NVS for WiFi stack
    ESP_ERROR_CHECK(nvs_flash_init());

    // Initialize the TCP/IP stack
    ESP_ERROR_CHECK(esp_netif_init());

    // Initialize the event loop
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // Create default WiFi station
    wifi_netif = esp_netif_create_default_wifi_sta();

    // Configure WiFi
    wifi_init_config_t wifi_init_config = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&wifi_init_config));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));

    // Set WiFi mode
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

    // Configure WiFi SSID and password
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASSWORD,
        },
    };

    // Set WiFi configuration
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));

    // Start WiFi
    ESP_ERROR_CHECK(esp_wifi_start());

    vTaskDelete(NULL);
}

uint8_t wifi_initialized(){
    return wifi_is_initialized;
}

uint8_t image_ready(){
    return image_is_ready;
}

uint8_t* get_image_buffer() {
    return image_buffer;
}

size_t get_image_size() {
    return image_size;
}

// void app_main(void)
// {
    
//  init_wifi();
   
// }
