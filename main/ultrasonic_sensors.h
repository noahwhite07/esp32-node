void ultrasonic_test(void *pvParameters);
uint8_t getTriggered();

// Define the callback function type
typedef void (*callback_t)(); 

// A struct for passing a callback into a FreeRTOS task
typedef struct {
    callback_t callback_function;
    // You can add more fields here if needed
} task_params_t;