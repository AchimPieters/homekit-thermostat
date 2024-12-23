set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DWOLFSSL_USER_SETTINGS")

idf_component_register(
    SRCS "main.c" "homekit.c" "relay.c" "sht40.c" "wifi.c" "err.c" "lcd.c" "gui.c"
    INCLUDE_DIRS "."
    REQUIRES freertos esp_wifi nvs_flash driver esp32-homekit sht4x
)
