#include <stdbool.h>
#include <driver/gpio.h>
#include <esp_log.h>

#define RELAY_GPIO_PIN 3 // TODO: set in Kconfig

bool relay_turned_on = false;

void relay_init() {
  gpio_set_direction(RELAY_GPIO_PIN, GPIO_MODE_OUTPUT);
}

void relay_on() {
  ESP_LOGI("INFORMATION", "Turning relay ON");
  gpio_set_level(RELAY_GPIO_PIN, 1);
  relay_turned_on = true;
}

void relay_off() {
  ESP_LOGI("INFORMATION", "Turning relay OFF");
  gpio_set_level(RELAY_GPIO_PIN, 0);
  relay_turned_on = false;
}