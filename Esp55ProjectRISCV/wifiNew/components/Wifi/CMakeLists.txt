idf_component_register(SRCS "WifiHandler.c" "mbedtlsApp.c"
                    INCLUDE_DIRS "."
                    REQUIRES mbedtls esp_wifi esp_event)
