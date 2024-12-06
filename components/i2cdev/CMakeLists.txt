set(req driver freertos esp_idf_lib_helpers)

idf_component_register(
    SRCS i2cdev.c
    INCLUDE_DIRS .
    REQUIRES ${req}
)