#include <stdio.h>
#include <esp_log.h>
#include <nvs_flash.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_err.h>
#include <esp_event.h>
#include "wifi.h"
#include "hw/relay.h"
#include "hw/sht40.h"
#include "hw/lcd.h"
#include "homekit.h"
#include "gui/gui.h"
#include "gui/scr_wifi_setup.h"
#include "gui/scr_loading.h"
#include "gui/scr_main.h"
#include "datetime.h"
#include "events.h"

 // TODO: increase to 30s / 1min?
#define TEMPERATURE_POLL_PERIOD 3000 // 3s

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

void on_wifi_reconnect_btn(void) {
  eventloop_dispatch(HOMEKIT_THERMOSTAT_WIFI_REQUEST_PROVISIONING, NULL, 0);
}

void on_eventloop_evt(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
  const char *tag = "EVENT";
  switch (event_id) {
    case HOMEKIT_THERMOSTAT_WIFI_REQUEST_PROVISIONING:
      // Init the provisioning and display the QR code
      ESP_LOGI(tag, "Received WiFi provisioning request.");
      char qr_data[150] = {0};
      wifi_init_provisioning(qr_data, sizeof(qr_data));
      // wifi_init_provisioning(qr_data, sizeof(qr_data)); // todo temp
      gui_wifi_scr(qr_data, sizeof(qr_data));

      // gui_loading_scr();
      // gui_loading_show_reconnect_btn(on_wifi_reconnect_btn);
      break;
    case HOMEKIT_THERMOSTAT_WIFI_CONN_ERROR:
      ESP_LOGI(tag, "WiFi connection error.");
      gui_loading_show_reconnect_btn(on_wifi_reconnect_btn);
      break;
    case HOMEKIT_THERMOSTAT_WIFI_CONNECTED:
      ESP_LOGI(tag, "WiFi connected.");

      // Start the Homekit server
      homekit_init(on_homekit_update);

      // TODO: load the initial data from homekit and update GUI (now it's stuck at loading until first action)

      // Fetch the current time from the internet
      datetime_init();

      // Start the temperature check task
      // xTaskCreate(task_temperature, "TempTask", configMINIMAL_STACK_SIZE * 3, NULL, 5, NULL);
      // xTaskCreate(task_datetime, "TimeTask", configMINIMAL_STACK_SIZE * 3, NULL, 5, NULL);

      eventloop_dispatch(HOMEKIT_THERMOSTAT_INIT_DONE, NULL, 0);
      break;
    case HOMEKIT_THERMOSTAT_INIT_STARTED:
      ESP_LOGI(tag, "Initialization started.");
      // TODO: remove provisioning screen from memory
      gui_loading_scr();

      // TODO: next steps
      // - update the logs string (dynamically grow string)
      // - handle go back to the wifi screen if needed
        // - provisioning manager - reset
        // - free logs string memory
        // - free loading screen GUI objectsfrcf
      break;
    case HOMEKIT_THERMOSTAT_INIT_UPDATE:
      if (event_data != NULL) {
        gui_loading_add_log((char*) event_data);
      }
      break;
    case HOMEKIT_THERMOSTAT_INIT_DONE:
      ESP_LOGI(tag, "Homekit thermostat is ready.");
      gui_main_scr();

      // TODO: define button handler

      // TODO: remove the event loop?
    break;

    // TODO: or can we somehow detect if WiFi goes down? --> it should restart when wifi connection is lost
  }
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

  // Init default event loop
  ESP_ERROR_CHECK(esp_event_loop_create_default());

  // Init Homekit Thermostat event loop
  eventloop_init(on_eventloop_evt);

  // Init WiFi
  wifi_init();

  if (!wifi_is_provisioned()) {
    eventloop_dispatch(HOMEKIT_THERMOSTAT_WIFI_REQUEST_PROVISIONING, NULL, 0);
  } else {
    eventloop_dispatch(HOMEKIT_THERMOSTAT_INIT_STARTED, NULL, 0);
    wifi_connect();
  }
}
