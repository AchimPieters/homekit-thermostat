#include <esp_err.h>
#include <esp_event.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <nvs_flash.h>
#include <stdio.h>

#include "datetime.h"
#include "events.h"
#include "gui/gui.h"
#include "gui/scr_loading.h"
#include "gui/scr_main.h"
#include "gui/scr_wifi_setup.h"
#include "homekit.h"
#include "hw/lcd.h"
#include "hw/relay.h"
#include "hw/sht40.h"
#include "wifi.h"

void task_temperature(void *pvParameters) {
  TempHumidity temp_humid;

  while (1) {
    temp_humid = sht40_measure_temp();
    ESP_LOGI("SHT40", "Humidity: %.1f%% Temperature: %.1fC", temp_humid.humidity, temp_humid.temperature);

    // Update temperature in homekit
    homekit_set_curr_temp(temp_humid);

    // Update temperature in GUI
    gui_set_curr_temp(temp_humid.temperature);

    HomekitState state = homekit_get_state();

    if (state.current_state == THERMOSTAT_HEAT) {
      // If the current temperature is higher that the target,
      // Turn off the relay and show the IDLE state on the display
      if (state.current_temp > state.target_temp && relay_turned_on) {
        ESP_LOGI("TEMP_TASK", "Current temperature (%.1f) is higher that the target (%.1f). Switching relay OFF", state.current_temp, state.target_temp);
        relay_off();
        gui_set_thermostat_status(_THERMOSTAT_IDLE);
      } else if (state.current_temp < state.target_temp && !relay_turned_on) {
        ESP_LOGI("TEMP_TASK", "Current temperature (%.1f) is lower that the target (%.1f). Switching relay ON", state.current_temp, state.target_temp);
        relay_on();
        gui_set_thermostat_status(THERMOSTAT_HEAT);
      }
    }

    vTaskDelay(pdMS_TO_TICKS(CONFIG_TEMPERATURE_POLL_PERIOD));
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

    vTaskDelay(pdMS_TO_TICKS(1000));  // 1s
  }
}

void on_homekit_update(HomekitState state) {
  printf("Update from homekit\n");
  printf("current state: %d\n", state.current_state);
  printf("target state: %d\n", state.target_state);
  printf("current temp: %1.f\n", state.current_temp);
  printf("target temp: %1.f\n", state.target_temp);

  // If the target state is set to either OFF or to one of unsupported states,
  // turn the thermostat off
  if (state.target_state == THERMOSTAT_OFF || state.target_state == _THERMOSTAT_AUTO || state.target_state == _THERMOSTAT_COOL) {
    homekit_set_thermostat_status(THERMOSTAT_OFF);
    relay_off();
    gui_set_thermostat_status(THERMOSTAT_OFF);

    return;
  }

  // Otherwise, make sure the current state is set to HEAT
  homekit_set_thermostat_status(THERMOSTAT_HEAT);

  if (state.current_temp < state.target_temp) {
    if (!relay_turned_on) {
      ESP_LOGI("HOMEKIT_UPDATE", "Current temperature (%.1f) is lower that the target (%.1f). Switching relay ON", state.current_temp, state.target_temp);
      relay_on();
    }
    gui_set_thermostat_status(THERMOSTAT_HEAT);
  } else if (state.current_temp > state.target_temp) {
    if (relay_turned_on) {
      ESP_LOGI("HOMEKIT_UPDATE", "Current temperature (%.1f) is higher that the target (%.1f). Switching relay OFF", state.current_temp, state.target_temp);
      relay_off();
    }
    gui_set_thermostat_status(_THERMOSTAT_IDLE);
  }

  // Update target temperature in GUI
  gui_set_target_temp(state.target_temp);
}

void on_wifi_reconnect_btn(void) {
  eventloop_dispatch(HOMEKIT_THERMOSTAT_WIFI_REQUEST_PROVISIONING, NULL, 0);
}

void on_temp_btn(ButtonType type) {
  float target_value = homekit_get_state().target_temp;

  // Update target temperature
  float new_temp = 0;
  if (type == BUTTON_INCREASE) {
    new_temp = target_value + 1;
    if (new_temp > CONFIG_THERMOSTAT_MAX_TEMP) {
      new_temp = CONFIG_THERMOSTAT_MAX_TEMP;
    }
  } else if (type == BUTTON_DECREASE) {
    new_temp = target_value - 1;

    if (new_temp < CONFIG_THERMOSTAT_MIN_TEMP) {
      new_temp = CONFIG_THERMOSTAT_MIN_TEMP;
    }
  }

  // Update homekit
  homekit_set_target_temp(new_temp);

  // Update GUI
  gui_set_target_temp(new_temp);
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

      // Fetch the current time from the internet
      datetime_init();

      eventloop_dispatch(HOMEKIT_THERMOSTAT_INIT_DONE, NULL, 0);
      break;
    case HOMEKIT_THERMOSTAT_INIT_STARTED:
      ESP_LOGI(tag, "Initialization started.");
      gui_loading_scr();
      break;
    case HOMEKIT_THERMOSTAT_INIT_UPDATE:
      if (event_data != NULL) {
        gui_loading_add_log((char *)event_data);
      }
      break;
    case HOMEKIT_THERMOSTAT_INIT_DONE:
      ESP_LOGI(tag, "Homekit thermostat is ready.");
      gui_main_scr();

      // Start the temperature check task
      xTaskCreate(task_temperature, "TempTask", configMINIMAL_STACK_SIZE * 3, NULL, 5, NULL);

      // Start the time update task
      xTaskCreate(task_datetime, "TimeTask", configMINIMAL_STACK_SIZE * 3, NULL, 5, NULL);

      // Set initial data in Homekit
      TempHumidity temp_humid = sht40_measure_temp();
      homekit_set_curr_temp(temp_humid);
      homekit_set_thermostat_status(THERMOSTAT_HEAT);

      // Show initial data in GUI
      HomekitState state = homekit_get_state();
      gui_set_curr_temp(temp_humid.temperature);
      gui_set_target_temp(state.target_temp);
      gui_set_thermostat_status(state.current_temp >= state.target_temp ? _THERMOSTAT_IDLE : THERMOSTAT_HEAT);

      // Register temperature buttons handler
      gui_on_btn_pressed_cb(on_temp_btn);
      break;
  }
}
// TODO: can we somehow detect if WiFi goes down? --> it should restart when wifi connection is lost

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
