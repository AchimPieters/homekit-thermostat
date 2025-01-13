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
#include "datetime.h"

 // TODO: increase to 30s / 1min?
#define TEMPERATURE_POLL_PERIOD 3000 // 3s
#define MIN_TEMP 10
#define MAX_TEMP 38 // Homekit allows up to 38 degrees

void task_temperature(void *pvParameters) {
  struct TempHumidity temp_humid;
  char temp_str[5];

  while (1) {
    temp_humid = sht40_measure_temp();
    ESP_LOGI("SHT40", "Humidity: %.1f%% Temperature: %.1fC", temp_humid.humidity, temp_humid.temperature);
    
    // Update temperature in homekit
    homekit_set_curr_temp(temp_humid);

    // Update temperature in GUI
    gui_set_curr_temp(temp_str);

    vTaskDelay(pdMS_TO_TICKS(TEMPERATURE_POLL_PERIOD));
  }
}

void task_datetime(void *pvParameters) {
  struct tm now;
  char time_buff[6];
  char date_buff[25];

  while (1) {
    now = datetime_now();
   
    datetime_timef(time_buff, sizeof(time_buff), &now);
    datetime_datef(date_buff, sizeof(date_buff), &now);
    gui_set_datetime(date_buff, time_buff);

    vTaskDelay(pdMS_TO_TICKS(1000)); // run every second
  }
}

void on_btn_touch(enum ButtonType type) {
  float target_value = homekit_get_target_temp();

  // Update target temperature
  float new_temp = 0;
  if (type == BUTTON_INCREASE) {
    new_temp = target_value + 1;
    if (new_temp > MAX_TEMP) {
      new_temp = MAX_TEMP;
    }
  } else if (type == BUTTON_DECREASE) {
    new_temp = target_value - 1;
    
    if (new_temp < MIN_TEMP) {
      new_temp = MIN_TEMP;
    }
  }  

  // Update homekit
  homekit_set_target_temp(new_temp);
  
  // Update GUI
  char temp_str[5];
  snprintf(temp_str, sizeof(temp_str), "%.1f", new_temp);
  gui_set_target_temp(temp_str);
}

void on_homekit_update(struct HomekitState state) {  
  // If the thermostat is set to HEAT and the current temperature is < than the target temperature
  // turn on the relay
  if (state.target_state == THERMOSTAT_HEAT && state.current_temp < state.target_temp) {
    if (state.current_state != THERMOSTAT_HEAT) {
      homekit_set_thermostat_status(THERMOSTAT_HEAT);
      relay_on();
    }
  } else {
    // Otherwise, if the relay is not already turned off, turn it off now
    if (state.current_state != THERMOSTAT_OFF) {
      homekit_set_thermostat_status(THERMOSTAT_OFF);
      relay_off();
    }
  }

  // Update target temperature in GUI
  char target_temp_str[5];
  snprintf(target_temp_str, sizeof(target_temp_str), "%.1f", state.target_temp);
  gui_set_target_temp(target_temp_str);

  // Update thermostat status in GUI
  char thermostat_status[20];
  homekit_get_thermostat_status(thermostat_status);
  gui_set_thermostat_status(thermostat_status);
}

void on_wifi_ready() {
  // Start the Homekit server
  homekit_init(on_homekit_update);

  // Fetch the current time from the internet
  datetime_init();

  // Start the temperature check task
  xTaskCreate(measure_temperature_task, "measure temperature", configMINIMAL_STACK_SIZE * 3, NULL, 5, NULL);
  xTaskCreate(task_datetime, "TimeTask", configMINIMAL_STACK_SIZE * 3, NULL, 5, NULL);
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

  // Initialize GUI data
  gui_set_datetime("-", "-");
  gui_set_target_temp("-");
  gui_set_curr_temp("-");
  gui_set_thermostat_status("Loading...");
  gui_on_btn_pressed_cb(&on_btn_touch);

  // Init Wifi connection
  wifi_init(&on_wifi_ready);
}
