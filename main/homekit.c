#include <stdio.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <homekit/homekit.h>
#include <homekit/characteristics.h>
#include <esp_err.h>
#include "homekit.h"
#include "relay.h"
#include "sht40.h"

static const char *TAG = "HOMEKIT";

void update_state();

void on_update(homekit_characteristic_t *ch, homekit_value_t value, void *context) {
  update_state();
}

homekit_characteristic_t name = HOMEKIT_CHARACTERISTIC_(NAME, THERMOSTAT_DEVICE_NAME);
homekit_characteristic_t manufacturer = HOMEKIT_CHARACTERISTIC_(MANUFACTURER, THERMOSTAT_DEVICE_MANUFACTURER);
homekit_characteristic_t serial = HOMEKIT_CHARACTERISTIC_(SERIAL_NUMBER, THERMOSTAT_DEVICE_SERIAL);
homekit_characteristic_t model = HOMEKIT_CHARACTERISTIC_(MODEL, THERMOSTAT_DEVICE_MODEL);
homekit_characteristic_t revision = HOMEKIT_CHARACTERISTIC_(FIRMWARE_REVISION, THERMOSTAT_FW_VERSION);

homekit_characteristic_t current_temperature = HOMEKIT_CHARACTERISTIC_(CURRENT_TEMPERATURE, 0);
homekit_characteristic_t current_humidity = HOMEKIT_CHARACTERISTIC_(CURRENT_RELATIVE_HUMIDITY, 0);
homekit_characteristic_t target_temperature = HOMEKIT_CHARACTERISTIC_(TARGET_TEMPERATURE, 20, .callback = HOMEKIT_CHARACTERISTIC_CALLBACK(on_update));
homekit_characteristic_t units = HOMEKIT_CHARACTERISTIC_(TEMPERATURE_DISPLAY_UNITS, 0);
homekit_characteristic_t current_state = HOMEKIT_CHARACTERISTIC_(CURRENT_HEATING_COOLING_STATE, 0);
homekit_characteristic_t target_state = HOMEKIT_CHARACTERISTIC_(TARGET_HEATING_COOLING_STATE, 0, .callback = HOMEKIT_CHARACTERISTIC_CALLBACK(on_update));
homekit_characteristic_t cooling_threshold = HOMEKIT_CHARACTERISTIC_(COOLING_THRESHOLD_TEMPERATURE, 25, .callback = HOMEKIT_CHARACTERISTIC_CALLBACK(on_update));
homekit_characteristic_t heating_threshold = HOMEKIT_CHARACTERISTIC_(HEATING_THRESHOLD_TEMPERATURE, 15, .callback = HOMEKIT_CHARACTERISTIC_CALLBACK(on_update));

void update_state() {
  uint8_t state = target_state.value.int_value;

  // If the thermostat is set to HEAT and the current temperature is < than the target temperature
  // turn on the relay
  if (state == THERMOSTAT_HEAT && current_temperature.value.float_value < target_temperature.value.float_value) {
    if (current_state.value.int_value != THERMOSTAT_HEAT) {
      current_state.value = HOMEKIT_UINT8(THERMOSTAT_HEAT);
      homekit_characteristic_notify(&current_state, current_state.value);
      ESP_LOGI(TAG, "Turning heating ON");
      relay_on();
    }
  } else {
    // Otherwise, if the relay is not already turned off, turn it off now
    if (current_state.value.int_value != THERMOSTAT_OFF) {
      current_state.value = HOMEKIT_UINT8(THERMOSTAT_OFF);
      homekit_characteristic_notify(&current_state, current_state.value);
      ESP_LOGI(TAG, "Turning heating OFF");
      relay_off();
    }
  }
}

void accessory_identify(homekit_value_t _value) {
  ESP_LOGI(TAG, "Accessory identified");
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Woverride-init"
homekit_accessory_t *accessories[] = {
  // .id = 1 -> https://github.com/maximkulkin/esp-homekit/issues/121
  HOMEKIT_ACCESSORY(.id = 1, .category = homekit_accessory_category_thermostats, .services = (homekit_service_t*[]) {
    HOMEKIT_SERVICE(ACCESSORY_INFORMATION, .characteristics = (homekit_characteristic_t*[]) {
      &name,
      &manufacturer,
      &serial,
      &model,
      &revision,
      HOMEKIT_CHARACTERISTIC(IDENTIFY, accessory_identify),
      NULL
    }),
    HOMEKIT_SERVICE(THERMOSTAT, .primary=true, .characteristics=(homekit_characteristic_t*[]) {
      HOMEKIT_CHARACTERISTIC(NAME, "HomeKit Thermostat"),
      &current_temperature,
      &target_temperature,
      &current_state,
      &target_state,
      &cooling_threshold,
      &heating_threshold,
      &units,
      &current_humidity,
      NULL
    }),
    NULL
  }),
  NULL
};
#pragma GCC diagnostic pop

homekit_server_config_t config = {
  .accessories = accessories,
  .password = CONFIG_ESP_SETUP_CODE,
  .setupId = CONFIG_ESP_SETUP_ID,
};

void homekit_init() {
  ESP_LOGI(TAG, "Starting HomeKit server...");
  homekit_server_init(&config);
}

void homekit_update_temperature(struct TempHumidity temp_humid) {
  current_temperature.value = HOMEKIT_FLOAT(temp_humid.temperature);
  current_humidity.value = HOMEKIT_FLOAT(temp_humid.humidity);

  homekit_characteristic_notify(&current_temperature, current_temperature.value);
  homekit_characteristic_notify(&current_humidity, current_humidity.value);
}