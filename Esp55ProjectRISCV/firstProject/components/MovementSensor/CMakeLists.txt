idf_component_register(
    SRCS "Port_Interrupt.c"
    INCLUDE_DIRS "."               # headers in same folder
    REQUIRES driver freertos esp_timer     # components this depends on
)
