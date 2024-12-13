#include <stdio.h>
#include <esp_log.h>
#include <nvs_flash.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_err.h>
#include "wifi.h"
#include "relay.h"
#include "err.h"
#include "sht40.h"
#include "homekit.h"

 // TODO: increase to 30s / 1min?
 // TODO: add this to kconfig
#define TEMPERATURE_POLL_PERIOD 3000 // 3s

void measure_temperature_task(void *pvParameters) {
  struct TempHumidity temp_humid;

  while (1) {
    temp_humid = sht40_measure_temp();
    ESP_LOGI("INFORMATION", "Humidity: %.1f%% Temperature: %.1fC", temp_humid.humidity, temp_humid.temperature);
    homekit_update_temperature(temp_humid);

    vTaskDelay(pdMS_TO_TICKS(TEMPERATURE_POLL_PERIOD));
  }
}

void on_wifi_ready() {
  // Start the Homekit server
  homekit_init();

  // Start the temperature check task
  xTaskCreate(measure_temperature_task, "measure temperature", configMINIMAL_STACK_SIZE * 3, NULL, 5, NULL);
}

void app_main() {
  // Init storage
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_LOGW("WARNING", "NVS flash initialization failed, erasing...");
    handle_err(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  handle_err(ret);

  // Init peripherals
  sht40_init();
  relay_init();

  // Init Wifi connection
  wifi_init(&on_wifi_ready);
}
