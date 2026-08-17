file(GLOB_RECURSE srcs "main.c" "src/*.c")

idf_component_register(SRCS "${srcs}" "portIsr/portIsr.c"
                       PRIV_REQUIRES bt nvs_flash esp_driver_gpio
                       INCLUDE_DIRS "./include")
