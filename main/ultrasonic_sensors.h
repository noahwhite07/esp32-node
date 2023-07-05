// Soon-to-be deprecated functions
void ultrasonic_test(void *pvParameters);
uint8_t getTriggered();




// Define the callback function type
typedef void (*callback_t)(); 

/*
    A struct for passing a callback into a FreeRTOS task
*/ 
typedef struct {
    callback_t callback_function;
} task_params_t;


/*
    This struct represents a pair of sensors connected to this node

    Params:

    trig: The common trigger pin of the sensor pair 
    echo_a: The echo pin of FIRST sensor to be tripped on crossing the pair INTO the zone
    echo_b: The echo pin of the SECOND sensor to be tripped on crossing the pair INTO the zone
    zone: the zone to which this sensor pair is an input (see diagram in documentation)

*/
typedef struct {
    uint8_t trig;
    uint8_t echo_a;
    uint8_t echo_b;
    uint8_t zone;
} sensor_pair_t;

/*
    A struct containing the parameters of the register_threshold function

    Params:
    
    sensor_pair: The pair of sensors comprising this threshold
    callback_function: The function to be called when a vehicle trips the threshold

*/

typedef struct {
    sensor_pair_t sensor_pair;
    callback_t callback_function;
} register_threshold_params_t;

/*
    A FreeRTOS task to register a pair of ultrasonic sensors as a threshold to a zone

    Starting this task will initialize each sensor and begin polling both sensors in the
    pair simultaneously. If a vehicle is detected as crossing the threshold, the passed
    in callback function will be called.

*/
void register_threshold(void *pvParameters);