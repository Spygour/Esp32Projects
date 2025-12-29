idf_component_register(SRCS "WifiHandler.c" "mbedtlsApp.c"
                       INCLUDE_DIRS "."
                       REQUIRES driver esp_wifi freertos mbedtls)
