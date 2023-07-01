#ifndef __XBEE_PLATFORM_ESP32
#define __XBEE_PLATFORM_ESP32

#ifndef XBEE_PLATFORM_HEADER 
    #define XBEE_PLATFORM_HEADER "../../ports/esp32/platform_config.h"
#endif


#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <inttypes.h>
#include <string.h>
#include <errno.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/gpio.h"

#define strcmpi         strcasecmp
#define strncmpi        strncasecmp

#define _f_memcpy memcpy
#define _f_memset memset

// macros used to declare a packed structure (no alignment of elements)
// The more-flexible XBEE_PACKED() replaced PACKED_STRUCT in 2019.
#define PACKED_STRUCT		struct __attribute__ ((__packed__))
#define XBEE_PACKED(name, decl)	PACKED_STRUCT name decl

// #define LITTLE_ENDIAN 4321
// #define BIG_ENDIAN 1234
// #define BYTE_ORDER LITTLE_ENDIAN

#define PACKED_STRUCT     struct __attribute__ ((__packed__))
#define XBEE_PACKED(name, decl) PACKED_STRUCT name decl

typedef uint8_t bool_t;

/*
    Note: THESE ARE FROM THE PERSPECTIVE OF THE ESP32!!!
    This means "cts_pin" is the pin on the ESP32 to which
    the XBee's RTS pin is connected!!!

    Flow ctrl pinout:

    XBee      |   ESP32
    ---------------------------
    RTS: DIO6  --> CTS: GPIO 15
    CTS: DIO7  --> RTS: GPIO 12
    ---------------------------

    Each device will use its RTS to output if it is ready to accept new data and read
    CTS to see if it is allowed to send data to the other device

    "Set RTS" = "Its okay for you to send data to me"
    "Get CTS" = "Is it okay for me to send data to you?"

*/ 
typedef struct xbee_serial_t {
    uart_port_t     uart_num;
    uint32_t        baudrate;
    uint8_t         tx_pin;
    uint8_t         rx_pin;
    uint8_t         cts_pin;
    uint8_t         rts_pin;
} xbee_serial_t;

#define ZCL_TIME_EPOCH_DELTA    ZCL_TIME_EPOCH_DELTA_1970
#define XBEE_MS_TIMER_RESOLUTION 10

    // enable the Wi-Fi code by default
    #ifndef XBEE_WIFI_ENABLED
        #define XBEE_WIFI_ENABLED 1
    #endif

    // enable the cellular code by default
    #ifndef XBEE_CELLULAR_ENABLED
        #define XBEE_CELLULAR_ENABLED 1
    #endif


#endif /* __XBEE_PLATFORM_ESP32 */

