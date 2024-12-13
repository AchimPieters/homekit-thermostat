#include <esp_system.h>
#include <esp_log.h>
#include <esp_err.h>

void handle_err(esp_err_t err) {
  if (err == ESP_OK) {
    return;
  }

  ESP_LOGE("ERROR", "Critical error: %s", esp_err_to_name(err));
  ESP_LOGE("ERROR", "Restarting device...");
  esp_restart();
}