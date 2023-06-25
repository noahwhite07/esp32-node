file(GLOB_RECURSE app_sources "src/wpan/*.c" "src/xbee/*.c" "ports/esp32/*.c" "src/util/*.c")
idf_component_register(SRCS "${app_sources}"
                    INCLUDE_DIRS "include" "ports/esp32"
                    PRIV_REQUIRES "driver" "freertos"
                    )
