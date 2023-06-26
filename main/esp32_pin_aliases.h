/*
This file contains definitions for pin names on the various ESP32 dev boards used for testing
This makes it easier to associate pin names displayed on the board with GPIO numbers used by the driver libs

*/

#define BOARD_NAME FEATHER

#ifdef BOARD_NAME
    // Adafruit HUZZAH32 ESP32 Feather
    #if BOARD_NAME == FEATHER
        #define PIN_LED 13
        #define PIN_A0 26
        #define PIN_A1 25
        #define PIN_A2 34 // NOT output capable 
        #define PIN_A3 39 // NOT output capable 
        #define PIN_A4 36 // NOT output capable 
        #define PIN_A5 4
        #define PIN_SCK 5
        #define PIN_MO 18
        #define PIN_MI 19
        #define PIN_RX 16
        #define PIN_TX 17
        #define PIN_SCL 22
        #define PIN_SDA 23
        
    
    #else
        #define LED_PIN 15
        
    #endif
#endif