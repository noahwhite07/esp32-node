// fetch_image.h

#ifndef FETCH_IMAGE_H
#define FETCH_IMAGE_H

#include <stddef.h>
#include <stdint.h>

void init_cameras();
uint8_t* get_image_buffer();
size_t get_image_size();
uint8_t cameras_initialized();
uint8_t image_ready();
void download_image(void *params);

/*
    Represents a connection to an ESP32-CAM
    module over WiFi
*/
typedef struct{
    uint8_t mac_addr[6];
    char server_url[16];
    uint8_t zone_number;
    uint8_t initialized;
}camera_t;  


// TODO: parameter should actually be a new image struct with a buffer and size
// This way the caller doesnt have to initialize them and pass them in
typedef struct{
    void (*callbackFunction)(uint8_t*, size_t, uint8_t);
    //camera_t* camera;
    uint8_t cam_zone_num;
}download_image_params_t;







#endif // FETCH_IMAGE_H
