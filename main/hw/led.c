#include <driver/gpio.h>

// Explicitly disable the builtin LED as it might sometimes turn on for some reason...
void led_disable() {
  gpio_reset_pin(GPIO_NUM_8);
  gpio_set_direction(GPIO_NUM_8, GPIO_MODE_OUTPUT);
  gpio_set_level(GPIO_NUM_8, 0);
}