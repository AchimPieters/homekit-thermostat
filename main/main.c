#include <stdio.h>
#include <esp_log.h>
#include <nvs_flash.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_err.h>
#include "wifi.h"
#include "relay.h"
#include "sht40.h"
#include "homekit.h"
#include "lcd.h"
#include "gui.h"

 // TODO: increase to 30s / 1min?
#define TEMPERATURE_POLL_PERIOD 3000 // 3s

void measure_temperature_task(void *pvParameters) {
  struct TempHumidity temp_humid;

  while (1) {
    temp_humid = sht40_measure_temp();
    ESP_LOGI("SHT40", "Humidity: %.1f%% Temperature: %.1fC", temp_humid.humidity, temp_humid.temperature);
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
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);

  // Init peripherals
  sht40_init();
  relay_init();

  // Init display
  lcd_init();
  gui_init();
  xTaskCreate(lvgl_timer_task, "LVGL timer", LVGL_TASK_STACK_SIZE, NULL, LVGL_TASK_PRIORITY, NULL);

  // Lock the mutex due to the LVGL APIs are not thread-safe
  if (lvgl_lock(-1)) {
    gui_render();
    lvgl_unlock();
  }

  // Init Wifi connection
  wifi_init(&on_wifi_ready);
}
