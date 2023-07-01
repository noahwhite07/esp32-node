idf_component_register(

    SRCS 
        #"read_sensor.c" 
        #"ultrasonic_sensors.c"
        #"send_image_with_sensors.c"
        "send_image.c"
        "fetch_image.c"
        "output.c" 
        "node_xbee.c"
        #"handleFrame.c"

    INCLUDE_DIRS "." 
)
