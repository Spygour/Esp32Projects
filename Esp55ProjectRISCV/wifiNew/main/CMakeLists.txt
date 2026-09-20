idf_component_register(SRCS "app_main.c"
                    REQUIRES Wifi MovementSensor DfRobotLidar7 EthHandler Spi nvs_flash
                    INCLUDE_DIRS ".")
