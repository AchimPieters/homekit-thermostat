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


  // Subtract 1°C as it has been noticed that the sensor is measuring increased values
  // This is a weird hack, I know, maybe this could be fixed with the sensor calibration
  // https://www.laskakit.cz/en/laskakit-sht40-senzor-teploty-a-vlhkosti-vzduchu/
  // but I'm too lazy to do that now, so let's see if this is good enough 😅
  temp_humid.temperature -= 1;

  return temp_humid;
}