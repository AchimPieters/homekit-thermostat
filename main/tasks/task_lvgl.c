#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "../gui/gui.h"

void task_lvgl(void *pvParameters) {
  ESP_LOGI("GUI", "Starting LVGL timer task");
  uint32_t task_delay_ms = LVGL_TASK_MAX_DELAY_MS;
  
  while (1) {
    // Lock the mutex due to the LVGL APIs are not thread-safe
    if (lvgl_lock(-1, "lv_timer_handler")) {
      task_delay_ms = lv_timer_handler();
      lvgl_unlock();
    }
    if (task_delay_ms > LVGL_TASK_MAX_DELAY_MS) {
      task_delay_ms = LVGL_TASK_MAX_DELAY_MS;
    } else if (task_delay_ms < LVGL_TASK_MIN_DELAY_MS) {
      task_delay_ms = LVGL_TASK_MIN_DELAY_MS;
    }
    vTaskDelay(pdMS_TO_TICKS(task_delay_ms));
  }
}