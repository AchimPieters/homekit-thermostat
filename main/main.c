#include <stdio.h>
#include <esp_log.h>
#include <nvs_flash.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <homekit/homekit.h>
#include <homekit/characteristics.h>
#include <esp_system.h>
#include <esp_err.h>

#include "wifi.h"
#include "relay.h"
#include "error_handler.h"
#include "sht40.h"

 // TODO: increase to 30s / 1min?
 // TODO: add this to kconfig
#define TEMPERATURE_POLL_PERIOD 10000 // 10s

void update_state();

void on_update(homekit_characteristic_t *ch, homekit_value_t value, void *context) {
  update_state();
}

homekit_characteristic_t current_temperature = HOMEKIT_CHARACTERISTIC_(CURRENT_TEMPERATURE, 0);
homekit_characteristic_t target_temperature = HOMEKIT_CHARACTERISTIC_(TARGET_TEMPERATURE, 20, .callback = HOMEKIT_CHARACTERISTIC_CALLBACK(on_update));
homekit_characteristic_t units = HOMEKIT_CHARACTERISTIC_(TEMPERATURE_DISPLAY_UNITS, 0);
homekit_characteristic_t current_state = HOMEKIT_CHARACTERISTIC_(CURRENT_HEATING_COOLING_STATE, 0);
homekit_characteristic_t target_state = HOMEKIT_CHARACTERISTIC_(TARGET_HEATING_COOLING_STATE, 0, .callback = HOMEKIT_CHARACTERISTIC_CALLBACK(on_update));
homekit_characteristic_t cooling_threshold = HOMEKIT_CHARACTERISTIC_(COOLING_THRESHOLD_TEMPERATURE, 25, .callback = HOMEKIT_CHARACTERISTIC_CALLBACK(on_update));
homekit_characteristic_t heating_threshold = HOMEKIT_CHARACTERISTIC_(HEATING_THRESHOLD_TEMPERATURE, 15, .callback = HOMEKIT_CHARACTERISTIC_CALLBACK(on_update));
homekit_characteristic_t current_humidity = HOMEKIT_CHARACTERISTIC_(CURRENT_RELATIVE_HUMIDITY, 0);

// Target Heating Cooling State (See 9.119 in Homekit Protocol PDF)
// 0 = OFF
// 1 = ”Heat. If the current temperature is below the target temperature then turn on heating.”
// 2 = ”Cool. If the current temperature is above the target temperature then turn on cooling.”
// 3 = ”Auto. Turn on heating or cooling to maintain temperature within the heating and cooling threshold of the target temperature.”
enum ThermostatStatus {
  THERMOSTAT_OFF = 0,
  THERMOSTAT_HEAT = 1,
  THERMOSTAT_COOL = 2,
  THERMOSTAT_AUTO = 3,
};

void update_state() {

  // TODO: rewrite and simplify this logic

  uint8_t state = target_state.value.int_value;
  if ((state == THERMOSTAT_HEAT && current_temperature.value.float_value < target_temperature.value.float_value) ||
      (state == THERMOSTAT_AUTO && current_temperature.value.float_value < heating_threshold.value.float_value))
  {
    if (current_state.value.int_value != THERMOSTAT_HEAT)
    {
      current_state.value = HOMEKIT_UINT8(THERMOSTAT_HEAT);
      homekit_characteristic_notify(&current_state, current_state.value);
      relay_on();
      ESP_LOGI("INFORMATION", "Heater On");
    }
  }
  else if ((state == THERMOSTAT_COOL && current_temperature.value.float_value > target_temperature.value.float_value) ||
           (state == THERMOSTAT_AUTO && current_temperature.value.float_value > cooling_threshold.value.float_value))
  {
    if (current_state.value.int_value != THERMOSTAT_COOL)
    {
      current_state.value = HOMEKIT_UINT8(THERMOSTAT_COOL);
      homekit_characteristic_notify(&current_state, current_state.value);
      relay_off();
      ESP_LOGI("INFORMATION", "Heater Off");
    }
  }
  else
  {
    if (current_state.value.int_value != THERMOSTAT_OFF)
    {
      current_state.value = HOMEKIT_UINT8(THERMOSTAT_OFF);
      homekit_characteristic_notify(&current_state, current_state.value);
      relay_off();
      ESP_LOGI("INFORMATION", "Everything Off");
    }
  }
}

void temperature_sensor_task(void *pvParameters) {
  float temperature_value = 19.2;
  float humidity_value = 49.9;

  while (1) {
    sht40_measure_temp();
    
    ESP_LOGI("INFORMATION", "Humidity: %.1f%% Temperature: %.1fC", humidity_value, temperature_value);

    current_temperature.value = HOMEKIT_FLOAT(temperature_value);
    current_humidity.value = HOMEKIT_FLOAT(humidity_value);

    homekit_characteristic_notify(&current_temperature, current_temperature.value);
    homekit_characteristic_notify(&current_humidity, current_humidity.value);

    vTaskDelay(pdMS_TO_TICKS(TEMPERATURE_POLL_PERIOD));
  }
}

// HomeKit characteristics
#define DEVICE_NAME "ESP32 Thermostat"
#define DEVICE_MANUFACTURER "Jozef Cipa"
// #define DEVICE_SERIAL "NLDA4SQN1466"
// #define DEVICE_MODEL "SD466NL/A"
#define DEVICE_SERIAL "THERMOSTAT-1"
#define DEVICE_MODEL "ESP32C6"
#define FW_VERSION "1.0"

homekit_characteristic_t name = HOMEKIT_CHARACTERISTIC_(NAME, DEVICE_NAME);
homekit_characteristic_t manufacturer = HOMEKIT_CHARACTERISTIC_(MANUFACTURER, DEVICE_MANUFACTURER);
homekit_characteristic_t serial = HOMEKIT_CHARACTERISTIC_(SERIAL_NUMBER, DEVICE_SERIAL);
homekit_characteristic_t model = HOMEKIT_CHARACTERISTIC_(MODEL, DEVICE_MODEL);
homekit_characteristic_t revision = HOMEKIT_CHARACTERISTIC_(FIRMWARE_REVISION, FW_VERSION);

// TODO: what's up with this? 
void accessory_identify(homekit_value_t _value) {
  ESP_LOGI("INFORMATION", "Accessory identify");
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Woverride-init"
homekit_accessory_t *accessories[] = {
  //  TODO: what is id = 1?
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

// TODO: pass this as an argument to wifi_init()
// void on_wifi_ready() {
//   ESP_LOGI("INFORMATION", "Starting HomeKit server...");
//   homekit_server_init(&config);
// }

void app_main() {
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_LOGW("WARNING", "NVS flash initialization failed, erasing...");
    CHECK_ERROR(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  CHECK_ERROR(ret);

  wifi_init();
  relay_init();
  sht40_init();

  xTaskCreate(temperature_sensor_task, "read data from sensor", configMINIMAL_STACK_SIZE * 3, NULL, 5, NULL);
}


// TODO: in main we will only init services and define tasks
// everything else will be in a separate file