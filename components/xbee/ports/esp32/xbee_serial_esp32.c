#include "esp_err.h"
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "xbee/platform.h"
#include "xbee/serial.h"
#include "driver/gpio.h"
#include "esp_log.h"

// default is for FAR to be ignored
#ifndef FAR
   #define FAR
#endif
#ifndef portTICK_RATE_MS
    #define portTICK_RATE_MS portTICK_PERIOD_MS
#endif

static const char *TAG = "xbee_serial_esp32";
void printBytesInHex(const char *buffer, int length);

// Have been tested

// A helper function for debugging
void printBytesInHex(const char *buffer, int length) {

    printf("{");
    for (int i = 0; i < length; i++) {
        printf("0x%02x", (unsigned char) buffer[i]);
        if (i < length - 1) {
            printf(", ");
        }
    }
    printf("}\n");
}

int xbee_ser_write( xbee_serial_t *serial, const void FAR *buffer, int length){
    //ESP_LOGI(TAG, "xbee_ser_write called here\n");
    if (serial == NULL || buffer == NULL) {
        return -EINVAL;
    }

    // Display the written bytes for debug purposes 
    //printBytesInHex((const char *)buffer, length);

    int bytes_written = uart_write_bytes(serial->uart_num, (const char *)buffer, length);
    if (bytes_written < 0) {
        return -EIO;
    }

    return bytes_written;
}

int xbee_ser_open( xbee_serial_t *serial, uint32_t baudrate){
    ESP_LOGI(TAG, "xbee_ser_open called here\n");
    if (serial == NULL) {
        return -EINVAL;
    }

    // Hardcoded pin config
    // serial->uart_num = 2;
    // const uint8_t TX_PIN = 17;
    // const uint8_t RX_PIN = 16; 
    // const uint8_t CTS_PIN = 12;
    // const uint8_t RTS_PIN = 22;


    // Hardcoded UART configuration
    // uart_config_t uart_config = {
    //     .baud_rate = baudrate,
    //     .data_bits = UART_DATA_8_BITS,
    //     .parity = UART_PARITY_DISABLE,
    //     .stop_bits = UART_STOP_BITS_1,
    //     .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    //     .source_clk = UART_SCLK_APB
    // };

    const uint8_t TX_PIN = serial->tx_pin;
    const uint8_t RX_PIN = serial->rx_pin; 
    const uint8_t CTS_PIN = serial->cts_pin;
    const uint8_t RTS_PIN = serial->rts_pin;

    uart_config_t uart_config = {
        .baud_rate = serial->baudrate,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_CTS_RTS,
        .source_clk = UART_SCLK_APB
    };

    // Configure and install the UART driver
    esp_err_t err = uart_param_config(serial->uart_num, &uart_config);
    if (err != ESP_OK) {
        return -EIO;
    }

    // Connect a pullup resistor to CTS so it's never floating
    gpio_set_direction(CTS_PIN, GPIO_MODE_INPUT);
    gpio_set_pull_mode(CTS_PIN, GPIO_PULLUP_ONLY);

    // Set up UART pins
    // Replace TX_PIN and RX_PIN with actual GPIO numbers for your hardware setup
    err = uart_set_pin(serial->uart_num, TX_PIN, RX_PIN, RTS_PIN, CTS_PIN);
    if (err != ESP_OK) {
        return -EIO;
    }

    // Install UART driver
    err = uart_driver_install(serial->uart_num, 256, 256, 0, NULL, 0);
    if (err != ESP_OK) {
        return -EIO;
    }

    // Update the baudrate field in the xbee_serial_t structure
    serial->baudrate = baudrate;

    ESP_LOGI(TAG, "Serial Pin Config\n");
    ESP_LOGI(TAG, "============================\n");
    ESP_LOGI(TAG, "UART NUM: %d\n", serial->uart_num);
    ESP_LOGI(TAG, "TX: %d\n", TX_PIN);
    ESP_LOGI(TAG, "RX: %d\n", RX_PIN);
    ESP_LOGI(TAG, "CTS: %d\n", CTS_PIN);
    ESP_LOGI(TAG, "RTS: %d\n", RTS_PIN);
    ESP_LOGI(TAG, "BAUD: %lu\n", baudrate);
    ESP_LOGI(TAG, "============================\n");


    return 0;
}

int xbee_ser_read( xbee_serial_t *serial, void FAR *buffer, int bufsize){
    //ESP_LOGI(TAG, "xbee_ser_read called here\n");

    if (serial == NULL || xbee_ser_invalid(serial) || buffer == NULL) {
        return -EINVAL;
    }

    int bytes_read = 0;
    int available = uart_read_bytes(serial->uart_num, (uint8_t *)buffer, bufsize, 10 / portTICK_PERIOD_MS);
    if (available > 0) {
        bytes_read = available;
    } else if (available < 0) {
        return -EIO;
    }

    return bytes_read;
}

bool_t xbee_ser_invalid( xbee_serial_t *serial){
    //ESP_LOGI(TAG, "xbee_ser_invalid called here\n");

    return 0;
}

int xbee_ser_putchar( xbee_serial_t *serial, uint8_t ch){
    //ESP_LOGI(TAG, "xbee_ser_putchar called here\n");

    if (serial == NULL || xbee_ser_invalid(serial)) {
        return -EINVAL;
    }

    // Try to write the character to the UART buffer with a short timeout
    int result = uart_write_bytes(serial->uart_num, (const char *)&ch, 1);

    if (result == 1) {
        return 0; // Successfully sent (queued) character
    } else {
        return -ENOSPC; // The write buffer is full and the character wasn't sent
    }
}

int xbee_ser_getchar( xbee_serial_t *serial){
    //ESP_LOGI(TAG, "xbee_ser_getchar called here\n");

    if (serial == NULL || xbee_ser_invalid(serial)) {
        return -EINVAL;
    }

    uint8_t ch;
    int result = uart_read_bytes(serial->uart_num, &ch, 1, 10 / portTICK_RATE_MS);

    if (result == 1) {
        return ch; // Return the character read from the XBee serial port
    } else {
        return -ENODATA; // There aren't any characters in the read buffer
    }
}

// Yet to be tested

int xbee_ser_flowcontrol(xbee_serial_t *serial, bool_t enabled) {
    ESP_LOGI(TAG, "xbee_ser_flowcontrol called here\n");
    ESP_LOGI(TAG, "Enabled: %d", enabled);

    // // if (serial == NULL) {
    // //     return -EINVAL;
    // // }

    // // esp_err_t err = uart_set_hw_flow_ctrl(serial->uart_num,
    // //                                       enabled ? UART_HW_FLOWCTRL_CTS_RTS : UART_HW_FLOWCTRL_DISABLE,
    // //                                       0);

    // // return err == ESP_OK ? 0 : -EIO;

    // // TODO: Put this back

    // Check for a valid serial port
    if (serial == NULL ) {
        return -EINVAL;
    }

    uart_port_t uart_num = serial->uart_num;
    uart_hw_flowcontrol_t flow_ctrl;

    // if (enabled) {
    //     // Enable hardware flow control (CTS/RTS)
    //     flow_ctrl = UART_HW_FLOWCTRL_CTS_RTS;
    // } else {
    //     // Disable hardware flow control
    //     flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
    // }

    flow_ctrl = UART_HW_FLOWCTRL_CTS_RTS;

    ESP_LOGI(TAG, "UART port number: %d", serial->uart_num);

    // Set the hardware flow control
    esp_err_t res = uart_set_hw_flow_ctrl(uart_num, flow_ctrl, 122 /*rx_flow_ctrl_thresh*/);

    if (res == ESP_OK) {
        return 0;
    } else {
        return -1;
    }

    //return 0; // Success
    
}

int xbee_ser_set_rts(xbee_serial_t *serial, bool_t asserted) {
    ESP_LOGI(TAG, "xbee_ser_set_rts called here\n");

    // if (serial == NULL) {
    //     return -EINVAL;
    // }

    // esp_err_t err = uart_set_rts(serial->uart_num, asserted ? 0 : 1);

    // return err == ESP_OK ? 0 : -EIO;

    // TODO: Put this back
    return 0;// do nothing
}

int xbee_ser_get_cts(xbee_serial_t *serial) {
    //ESP_LOGI(TAG, "xbee_ser_get_cts called here\n");

    // if (serial == NULL) {
    //     return -EINVAL;
    // }

    //return gpio_get_level(serial->cts_pin);

    if (serial == NULL) {
        return -EINVAL;
    }
    
    int level = gpio_get_level(serial->cts_pin);
    
    //ESP_LOGI(TAG, "CTS level: %d\n", level);

    // CTS is active low, so invert the logic level
    return !level;


    // TODO: put this back the way it was
    //return 1;
}

const char *xbee_ser_portname( xbee_serial_t *serial){
    //ESP_LOGI(TAG, "xbee_ser_portname called here\n");

    static const char *uart_port_names[] = {
        "UART0",
        "UART1",
        "UART2"
    };

    if (serial == NULL || xbee_ser_invalid(serial) || serial->uart_num >= UART_NUM_MAX) {
        return "(invalid)";
    }

    return uart_port_names[serial->uart_num];
}

int xbee_ser_close(xbee_serial_t *serial) {
   // ESP_LOGI(TAG, "xbee_ser_close called here\n");

    if (serial == NULL || xbee_ser_invalid(serial)) {
        return -EINVAL;
    }

    esp_err_t err = uart_driver_delete(serial->uart_num);
    if (err == ESP_OK) {
        serial->uart_num = UART_NUM_MAX; // Set to an invalid UART number to mark as closed.
        return 0;
    } else if (err == ESP_ERR_INVALID_ARG) {
        return -EINVAL;
    } else {
        return -EIO;
    }
}

int xbee_ser_baudrate(xbee_serial_t *serial, uint32_t baudrate) {
    //ESP_LOGI(TAG, "xbee_ser_baudrate called here\n");

    if (serial == NULL || xbee_ser_invalid(serial)) {
        return -EINVAL;
    }

    esp_err_t err = uart_set_baudrate(serial->uart_num, baudrate);
    if (err == ESP_OK) {
        serial->baudrate = baudrate;
        return 0;
    } else if (err == ESP_ERR_INVALID_ARG) {
        return -EINVAL;
    } else {
        return -EIO;
    }
}

int xbee_ser_break( xbee_serial_t *serial, bool_t enabled){
    ESP_LOGI(TAG, "xbee_ser_break called here\n");

    if (serial == NULL) {
        return -EINVAL;
    }

    gpio_config_t tx_config;
    tx_config.pin_bit_mask = 1ULL << serial->tx_pin;
    tx_config.mode = GPIO_MODE_OUTPUT;
    tx_config.pull_up_en = GPIO_PULLUP_DISABLE;
    tx_config.pull_down_en = GPIO_PULLDOWN_DISABLE;
    tx_config.intr_type = GPIO_INTR_DISABLE;

    esp_err_t err = gpio_config(&tx_config);
    if (err != ESP_OK) {
        return -EIO;
    }

    if (enabled) {
        gpio_set_level(serial->tx_pin, 0);
    } else {
        gpio_set_level(serial->tx_pin, 1);

        // Reconfigure the TX pin as a UART pin
        err = uart_set_pin(serial->uart_num, serial->tx_pin, serial->rx_pin, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
        if (err != ESP_OK) {
            return -EIO;
        }
    }

    return 0;
}

int xbee_ser_tx_free(xbee_serial_t *serial) {
    //ESP_LOGI(TAG, "xbee_ser_tx_free called here\n");

    // if (serial == NULL) {
    //     return -EINVAL;
    // }

    size_t tx_free;
    esp_err_t err = uart_get_tx_buffer_free_size(serial->uart_num, &tx_free);

    //ESP_LOGI(TAG, "TX Free: %zu", tx_free);
    if (err != ESP_OK) {
        return -EIO;
    }

    return tx_free;

    // TODO: Put this back
    //return (int)(UINT16_MAX);

    // size_t buffered_size;
    // esp_err_t ret = uart_get_buffered_data_len(UART_NUM_2, &buffered_size);
    
    // if (ret == ESP_OK) {
    //     printf("Number of bytes in the UART2 TX buffer: %d\n", buffered_size);
    // } else {
    //     printf("Error getting the buffered data length\n");
    // }

    // return buffered_size;

}

int xbee_ser_tx_used(xbee_serial_t *serial) {

    size_t buffered_size;
    esp_err_t ret = uart_get_buffered_data_len(UART_NUM_2, &buffered_size);

    if (ret == ESP_OK) {
        //ESP_LOGI(TAG, "TX Used: %zu", buffered_size);
    } else {
        printf("Error getting the buffered data length\n");
    }

    return buffered_size;

    //ESP_LOGI(TAG, "xbee_ser_tx_used called here\n");

    // if (serial == NULL) {
    //     return -EINVAL;
    // }

    // size_t tx_free;
    // esp_err_t err = uart_get_tx_buffer_free_size(serial->uart_num, &tx_free);
    // if (err != ESP_OK) {
    //     return -EIO;
    // }

    // // Assuming you used 256 as the total buffer size in xbee_ser_open function
    // // If you change the total buffer size there, make sure to update this value as well
    // size_t total_buffer_size = 256;
    // size_t tx_used = total_buffer_size - tx_free;

    // return tx_used;

    // TODO: Put this back
    return 0;
}

int xbee_ser_tx_flush(xbee_serial_t *serial) {
    ESP_LOGI(TAG, "xbee_ser_tx_flush called here\n");

    if (serial == NULL) {
        return -EINVAL;
    }

    esp_err_t err = uart_flush_input(serial->uart_num);
    if (err != ESP_OK) {
        return -EIO;
    }

    return 0;
}


// Yet to be implemented

int xbee_ser_rx_flush( xbee_serial_t *serial){
    ESP_LOGI(TAG, "xbee_ser_rx_flush called here\n");

    return 0; // Do nothing
}
int xbee_ser_rx_used( xbee_serial_t *serial){
    //ESP_LOGI(TAG, "xbee_ser_rx_used called here\n");

    return 0;

}
int xbee_ser_rx_free( xbee_serial_t *serial){
    //ESP_LOGI(TAG, "xbee_ser_rx_free called here\n");

    return (int)(UINT16_MAX);

}

