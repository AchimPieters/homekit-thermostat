set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DWOLFSSL_USER_SETTINGS")

file(GLOB SRC_FILES 
    "*.c"
    "gui/*.c"
    "hw/*.c"
)

idf_component_register(
    SRCS ${SRC_FILES}
    INCLUDE_DIRS "."
    REQUIRES freertos esp_wifi nvs_flash driver esp32-homekit sht4x lv_qrcode
)
