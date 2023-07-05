// fetch_image.h

#ifndef FETCH_IMAGE_H
#define FETCH_IMAGE_H

#include <stddef.h>
#include <stdint.h>

void init_wifi();
uint8_t* get_image_buffer();
size_t get_image_size();
uint8_t wifi_initialized();
uint8_t image_ready();
void download_image(void *params);

typedef struct{
    void (*callbackFunction)(void);
}download_image_params_t;

#endif // FETCH_IMAGE_H
