set(srcs "main.c")

idf_component_register(SRCS "ble_cts_cent.c" "${srcs}"
                       INCLUDE_DIRS ".")
