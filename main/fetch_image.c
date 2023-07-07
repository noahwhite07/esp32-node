#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_netif.h"
#include "nvs_flash.h"
#include "esp_http_client.h"
#include <string.h>

#include "fetch_image.h"

#define TAG "fetch_image"
#define NUM_CAMERAS 3

// Declare global pointers for cameras
camera_t* cam_1;
camera_t* cam_2;
camera_t* cam_3;

// Declare an array to hold the pointers
camera_t* cameras[NUM_CAMERAS];

// Keep track of how many cameras have been connected and assigned IP addresses
uint8_t num_cams_initialized = 0;

// Store the most recently fetched image
static uint8_t *image_buffer = NULL; // init wifi, wait for wifi initialized, download image, wait for image ready, send image
static size_t image_size = 0;

static uint8_t cameras_are_initialized = 0;
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

static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    if (event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
        ESP_LOGI(TAG, "Station connected");
        //ESP_LOGI(TAG, MACSTR, MAC2STR(event->mac));


        // Compare the MAC addresses
        // if (memcmp(event->mac, cam_1->mac_addr, 6) == 0) {
        //     ESP_LOGI(TAG, "Matched with camera 1");
        // }else{
        //     ESP_LOGI(TAG, "No matching MAC address found");

        // }

    // TODO: Remove the URL in the camera struct when it has disconnected from the access point
    } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
        ESP_LOGI(TAG, "Station disconnected");
        //ESP_LOGI(TAG, MACSTR, MAC2STR(event->mac));
    }
}

static void ip_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    if (event_id == IP_EVENT_AP_STAIPASSIGNED) {
        ip_event_ap_staipassigned_t* event = (ip_event_ap_staipassigned_t*) event_data;
        ESP_LOGI(TAG, "Station assigned IP:" );
        ESP_LOGI(TAG, IPSTR, IP2STR(&event->ip));

        //Loop through the cameras and compare MAC addresses
        for (int i = 0; i < NUM_CAMERAS; i++) {
            if (memcmp(event->mac, cameras[i]->mac_addr, 6) == 0) {
                snprintf(cameras[i]->server_url, 16, IPSTR, IP2STR(&event->ip));
                ESP_LOGI(TAG, "IP address stored for camera %d", i + 1);
                cameras[i]->initialized = 1;       
            }
        }

        // Check how many of the cameras have been assigned an IP address
        uint8_t cams_initialized = 0;
        for (int i = 0; i < NUM_CAMERAS; i++) {
            if(cameras[i]->initialized){
                ESP_LOGI(TAG,"Camera %d has been assigned an IP address", (i+1));
                cams_initialized ++;
            }
        }

        // When every camera has been assigned an IP address, we consider the wifi connections to be initialized
        if(cams_initialized == NUM_CAMERAS){
            // We might want to put a small delay here to give the last camera's web server time to start
            cameras_are_initialized = 1;
        }

    }
}

void init_cameras(){
    //Allocate space for each camera struct and associate the physical address with its zone number
    cam_1 = (camera_t*) malloc(sizeof(camera_t));
    uint8_t mac_1[6] = {0xa0, 0xb7, 0x65, 0x4b, 0xa8, 0xd4};
    memcpy(cam_1->mac_addr, mac_1, 6);
    cam_1->zone_number = 1;
    cam_1->initialized = 0;

    cam_2 = (camera_t*) malloc(sizeof(camera_t));
    uint8_t mac_2[6] = {0xb0, 0xb2, 0x1c, 0x09, 0xde, 0xec};
    memcpy(cam_2->mac_addr, mac_2, 6);
    cam_2->zone_number = 2;
    cam_2->initialized = 0;

    cam_3 = (camera_t*) malloc(sizeof(camera_t));
    uint8_t mac_3[6] = {0xe0, 0x5a, 0x1b, 0xa5, 0xe7, 0xd8};
    memcpy(cam_3->mac_addr, mac_3, 6);
    cam_3->zone_number = 3;
    cam_3->initialized = 0;

    // TODO: obtain 4th camera and allocate it here

    // Place the camera pointers into a global array so they can be iterated through 
    cameras[0] = cam_1;
    cameras[1] = cam_2;
    cameras[2] = cam_3;

    // Initialize NVS
    ESP_ERROR_CHECK(nvs_flash_init());

    // Initialize TCP/IP
    ESP_ERROR_CHECK(esp_netif_init());

    // Initialize the event loop
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // Create default WiFi AP
    esp_netif_create_default_wifi_ap();

    // Initialize WiFi with default config
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // Register IP event handler
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_AP_STAIPASSIGNED, &ip_event_handler, NULL));

    // Register WiFi event handler
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));

    // Configure WiFi as AP
    wifi_config_t wifi_config = {
        .ap = {
            .ssid = "Central_Node_AP",
            .ssid_len = strlen("Central_Node_AP"),
            .password = "password",
            .max_connection = 4,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK
        }
    };

    if (strlen("password") == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    // Set WiFi mode and config
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config));
    
    // Start WiFi
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "Central Node initialized as AP");

    vTaskDelete(NULL);
}

uint8_t cameras_initialized(){
    return cameras_are_initialized;
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

void download_image(void *params)
{   
    // Extract the passed in callback function from the params struct
    download_image_params_t* download_params = (download_image_params_t*) params;

    // Extract the camera from which to fetch an image
    // camera_t* cam = download_params->camera;
    // char* server_url = cam->server_url;
    uint8_t zone_number = download_params->cam_zone_num;




    //===========================================================================//
    // Download an image from the camera's web server
    //===========================================================================//

    ESP_LOGI(TAG, "Starting download task");
    image_is_ready = 0;

    char full_url[50]; // buffer to store the full URL

    // Format the full URL of the esp32-cam's web server endpoint
    sprintf(full_url, "http://%s/capture", cameras[zone_number - 1]->server_url);

    // Configure the HTTP client
    esp_http_client_config_t config = {
        .url = full_url, // use the formatted URL
        .event_handler = http_event_handler,
    };


    esp_http_client_handle_t client = esp_http_client_init(&config);

    // Make the HTTP GET request
    esp_err_t err = esp_http_client_perform(client);
    
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Image downloaded successfully from camera %d", zone_number); 
    } else {
        ESP_LOGE(TAG, "HTTP GET request failed: %s", esp_err_to_name(err));
    }
    
    // Alert the passed in callback function that the image is done downloading
    if (download_params && download_params->callbackFunction) {
        ESP_LOGI(TAG, "calling callback function");
        download_params->callbackFunction(image_buffer, image_size);
    }else{
        ESP_LOGI(TAG, "NULL Callback function");
    }

    esp_http_client_cleanup(client);


    vTaskDelete(NULL);
}
