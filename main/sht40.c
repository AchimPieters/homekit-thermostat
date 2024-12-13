#include <sht4x.h>
#include <string.h>

// TODO: define these in Kconfig
#define CONFIG_EXAMPLE_I2C_MASTER_SDA 6
#define CONFIG_EXAMPLE_I2C_MASTER_SCL 7

static sht4x_t sensor;

void sht40_init() {
  ESP_ERROR_CHECK(i2cdev_init());

  memset(&sensor, 0, sizeof(sht4x_t));

  ESP_ERROR_CHECK(sht4x_init_desc(&sensor, 0, CONFIG_EXAMPLE_I2C_MASTER_SDA, CONFIG_EXAMPLE_I2C_MASTER_SCL));
  ESP_ERROR_CHECK(sht4x_init(&sensor));
}

void sht40_measure_temp() {
  float temperature;
  float humidity;

  // perform one measurement and do something with the results
  ESP_ERROR_CHECK(sht4x_measure(&sensor, &temperature, &humidity));
  printf("sht4x Sensor: %.2f °C, %.2f %%\n", temperature, humidity);
}