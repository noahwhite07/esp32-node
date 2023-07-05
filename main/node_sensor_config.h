/*
    This file contains the pin connections from the ultrasonic ports
    oboard the node PCB to the ESP32's GPIO pin numbers.

    Eventually, this should also contain the relationships between 
    the sensor pairs and the zone threshold to which they correspond. 


    Note that S1 specifically was routed incorrectly, so this configuration
    is not consistent with the schematic, but it is correct.

    Connections between the ultrasonic sensors and the node's terminals should be as follows:


    switch S3_ECHO_A with S1_TRIG

    VCC: orange
    GND: blue
    TRIG: yellow
    ECHO1: green
    ECHO2 red
*/ 


//======================================================================//
// S1
//======================================================================//

#define S1_ECHO_A 39 // sensor_vn = gpio39
#define S1_ECHO_B 34 // sensor_vp = gpio36
#define S1_TRIG  33 // This is actually S3::EA on the silkscreen

//======================================================================//
// S2
//======================================================================//

#define S2_ECHO_A 35
#define S2_ECHO_B 32
#define S2_TRIG 22

//======================================================================//
// S3
//======================================================================//

#define S3_ECHO_A 36 // This is actually S1::TRIG on the silkscreen
#define S3_ECHO_B 25
#define S3_TRIG 0

//======================================================================//
// S4
//======================================================================//

#define S4_ECHO_A 26
#define S4_ECHO_B 27
#define S4_TRIG 23