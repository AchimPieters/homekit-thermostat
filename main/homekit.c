#include <stdio.h>
#include <string.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <homekit/homekit.h>
#include <homekit/characteristics.h>
#include <esp_err.h>
#include "homekit.h"
#include "events.h"

static const char *TAG = "HOMEKIT";

// Callback handler when Homekit update arrives
static void (*on_homekit_update_cb)(HomekitState state);

static void on_homekit_update(homekit_characteristic_t *ch, homekit_value_t value, void *context);

static homekit_characteristic_t name = HOMEKIT_CHARACTERISTIC_(NAME, THERMOSTAT_DEVICE_NAME);
static homekit_characteristic_t manufacturer = HOMEKIT_CHARACTERISTIC_(MANUFACTURER, THERMOSTAT_DEVICE_MANUFACTURER);
static homekit_characteristic_t serial = HOMEKIT_CHARACTERISTIC_(SERIAL_NUMBER, THERMOSTAT_DEVICE_SERIAL);
static homekit_characteristic_t model = HOMEKIT_CHARACTERISTIC_(MODEL, THERMOSTAT_DEVICE_MODEL);
static homekit_characteristic_t revision = HOMEKIT_CHARACTERISTIC_(FIRMWARE_REVISION, THERMOSTAT_FW_VERSION);

static homekit_characteristic_t current_temperature = HOMEKIT_CHARACTERISTIC_(CURRENT_TEMPERATURE, 0);
static homekit_characteristic_t current_humidity = HOMEKIT_CHARACTERISTIC_(CURRENT_RELATIVE_HUMIDITY, 0);
static homekit_characteristic_t target_temperature = HOMEKIT_CHARACTERISTIC_(TARGET_TEMPERATURE, 20, .callback = HOMEKIT_CHARACTERISTIC_CALLBACK(on_homekit_update));
static homekit_characteristic_t units = HOMEKIT_CHARACTERISTIC_(TEMPERATURE_DISPLAY_UNITS, 0);
static homekit_characteristic_t current_state = HOMEKIT_CHARACTERISTIC_(CURRENT_HEATING_COOLING_STATE, THERMOSTAT_HEAT);
static homekit_characteristic_t target_state = HOMEKIT_CHARACTERISTIC_(TARGET_HEATING_COOLING_STATE, THERMOSTAT_HEAT, .callback = HOMEKIT_CHARACTERISTIC_CALLBACK(on_homekit_update));
static homekit_characteristic_t cooling_threshold = HOMEKIT_CHARACTERISTIC_(COOLING_THRESHOLD_TEMPERATURE, 25, .callback = HOMEKIT_CHARACTERISTIC_CALLBACK(on_homekit_update));
static homekit_characteristic_t heating_threshold = HOMEKIT_CHARACTERISTIC_(HEATING_THRESHOLD_TEMPERATURE, 15, .callback = HOMEKIT_CHARACTERISTIC_CALLBACK(on_homekit_update));

HomekitState homekit_get_state() {
  HomekitState state = {
    .current_state = current_state.value.int_value,
    .target_state = target_state.value.int_value,
    .current_temp = current_temperature.value.float_value,
    .target_temp = target_temperature.value.float_value,
  };

  return state;
}

static void on_homekit_update(homekit_characteristic_t *ch, homekit_value_t value, void *context) {
  if (on_homekit_update_cb != NULL) {
    HomekitState state = homekit_get_state();
    on_homekit_update_cb(state);
  }
}

void accessory_identify(homekit_value_t _value) {
  ESP_LOGI(TAG, "Accessory identified");
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Woverride-init"
static homekit_accessory_t *accessories[] = {
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

static homekit_server_config_t config = {
  .accessories = accessories,
  .password = CONFIG_HOMEKIT_SETUP_CODE,
  .setupId = CONFIG_HOMEKIT_SETUP_ID,
};

void homekit_init(void (*on_homekit_update)(HomekitState state)) {
  // register callback
  on_homekit_update_cb = on_homekit_update;

  char *msg = "Starting HomeKit server...";
  eventloop_dispatch(HOMEKIT_THERMOSTAT_LOG, msg, strlen(msg) + 1);
  
  homekit_server_init(&config);
}

void homekit_set_curr_temp(TempHumidity temp_humid) {
  current_temperature.value = HOMEKIT_FLOAT(temp_humid.temperature);
  current_humidity.value = HOMEKIT_FLOAT(temp_humid.humidity);

  homekit_characteristic_notify(&current_temperature, current_temperature.value);
  homekit_characteristic_notify(&current_humidity, current_humidity.value);
}

void homekit_set_target_temp(float temp) {
  ESP_LOGI(TAG, "Setting target temperature to %.1f°C", temp);
  target_temperature.value = HOMEKIT_FLOAT(temp);
  homekit_characteristic_notify(&target_temperature, target_temperature.value);
}

void homekit_set_thermostat_status(ThermostatStatus status) {
  current_state.value = HOMEKIT_UINT8(status);
  homekit_characteristic_notify(&current_state, current_state.value);
}
