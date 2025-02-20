#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "../gui/scr_main.h"
#include "../homekit.h"
#include "../hw/relay.h"
#include "../hw/sht40.h"

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