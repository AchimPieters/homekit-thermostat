#include <stdbool.h>
#include <driver/gpio.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>

static const char *TAG = "RELAY";

bool relay_turned_on = false;

void relay_init() {
  gpio_config_t io_conf = {
    .pin_bit_mask = (1ULL << CONFIG_RELAY_PIN),
    .mode = GPIO_MODE_OUTPUT,
    .pull_up_en = GPIO_PULLUP_DISABLE,
    .pull_down_en = GPIO_PULLDOWN_DISABLE,
    .intr_type = GPIO_INTR_DISABLE
  };

  gpio_config(&io_conf);
  gpio_set_level(CONFIG_RELAY_PIN, 0);
}

void relay_on() {
  ESP_LOGI(TAG, "Turning relay ON");
  gpio_set_level(CONFIG_RELAY_PIN, 1);
  relay_turned_on = true;
}

void relay_off() {
  ESP_LOGI(TAG, "Turning relay OFF");
  gpio_set_level(CONFIG_RELAY_PIN, 0);
  relay_turned_on = false;
}