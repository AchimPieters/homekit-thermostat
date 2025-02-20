#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "../datetime.h"
#include "../gui/scr_main.h"

void task_time(void *pvParameters) {
  struct tm now;
  char time_buff[6];
  char date_buff[25];

  while (1) {
    now = datetime_now();

    datetime_timef(time_buff, sizeof(time_buff), &now);
    datetime_datef(date_buff, sizeof(date_buff), &now);
    gui_set_datetime(date_buff, time_buff);

    vTaskDelay(pdMS_TO_TICKS(1000));  // 1s
  }
}