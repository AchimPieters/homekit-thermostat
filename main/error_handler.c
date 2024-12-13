#include <esp_wifi.h>
#include <esp_system.h>
#include <esp_log.h>
#include "wifi.h"

void handle_error(esp_err_t err) {
  switch (err) {
    case ESP_ERR_WIFI_NOT_STARTED:
    case ESP_ERR_WIFI_CONN:
      wifi_restart();
      break;
    default:
      ESP_LOGE("ERROR", "Critical error, restarting device...");
      esp_restart();
      break;
  }
}