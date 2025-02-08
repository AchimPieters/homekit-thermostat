#include <sht4x.h>
#include <string.h>
#include <esp_err.h>
#include <esp_log.h>
#include "sht40.h"

// TODO: define these in Kconfig
#define CONFIG_EXAMPLE_I2C_MASTER_SDA 6
#define CONFIG_EXAMPLE_I2C_MASTER_SCL 7

sht4x_t sensor;

void sht40_init() {
  ESP_ERROR_CHECK(i2cdev_init());

  memset(&sensor, 0, sizeof(sht4x_t));

  ESP_ERROR_CHECK(sht4x_init_desc(&sensor, 0, CONFIG_EXAMPLE_I2C_MASTER_SDA, CONFIG_EXAMPLE_I2C_MASTER_SCL));
  ESP_ERROR_CHECK(sht4x_init(&sensor));

  ESP_LOGI("SHT40", "Temperature sensor initialized.");
}

TempHumidity sht40_measure_temp() {
  TempHumidity temp_humid = {};
  ESP_ERROR_CHECK(sht4x_measure(&sensor, &temp_humid.temperature, &temp_humid.humidity));

  return temp_humid;
}