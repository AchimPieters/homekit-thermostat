#include <sht4x.h>
#include <string.h>
#include <esp_err.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include "sht40.h"

static sht4x_t sensor;

void sht40_init() {
  ESP_ERROR_CHECK(i2cdev_init());

  memset(&sensor, 0, sizeof(sht4x_t));

  ESP_ERROR_CHECK(sht4x_init_desc(&sensor, 0, CONFIG_SHT40_I2C_SDA, CONFIG_SHT40_I2C_SCL));
  ESP_ERROR_CHECK(sht4x_init(&sensor));

  ESP_LOGI("SHT40", "Temperature sensor initialized.");
}

TempHumidity sht40_measure_temp() {
  TempHumidity temp_humid = {};
  ESP_ERROR_CHECK(sht4x_measure(&sensor, &temp_humid.temperature, &temp_humid.humidity));

  return temp_humid;
}