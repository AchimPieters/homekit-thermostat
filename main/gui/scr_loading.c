#include "scr_loading.h"

#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <lvgl.h>

#include "gui.h"

#define MAX_LOGS 30
#define MAX_LOG_SIZE 100

static const char *TAG = "LOADING SCREEN";

static lv_obj_t *scr_loading;
static lv_obj_t *logs_cont;
static lv_obj_t *lbl_logs;
static lv_obj_t *btn_cont;

static char logs[MAX_LOGS][MAX_LOG_SIZE] = {0};
static char log_str[(MAX_LOGS * MAX_LOG_SIZE) + MAX_LOGS /* MAX_LOGS * 1 (\n) */ + 1 /* \0 */] = "";

void gui_loading_scr() {
  ESP_LOGI(TAG, "Rendering");

  // reset logs if we come to the loading screen from a different screen
  if (gui_active_scr != scr_loading && lbl_logs != NULL) {
    log_str[0] = '\0';
    lv_label_set_text(lbl_logs, log_str);
  }

  if (scr_loading == NULL) {
    if (!lvgl_lock(-1, "gui_loading_scr")) {
      ESP_LOGE(TAG, "Failed to acquire lock");
      return;
    }

    // create main flexbox row container
    scr_loading = lv_obj_create(NULL);
    lv_obj_set_size(scr_loading, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_pad_all(scr_loading, 10, LV_PART_MAIN);
    lv_obj_set_flex_flow(scr_loading, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_border_width(scr_loading, 0, LV_PART_MAIN);

    // create a logs container
    logs_cont = lv_obj_create(scr_loading);
    lv_obj_set_flex_grow(logs_cont, 2);
    lv_obj_set_width(logs_cont, LV_PCT(100));
    lv_obj_set_style_border_width(logs_cont, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(logs_cont, 0, LV_PART_MAIN);

    // add a logs label
    lbl_logs = lv_label_create(logs_cont);
    lv_label_set_recolor(lbl_logs, true);
    lv_label_set_text(lbl_logs, "");
    lv_label_set_long_mode(lbl_logs, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(lbl_logs, lv_pct(100));

    // this must be last thing called before showing the screen
    lvgl_unlock();
  }

  // show screen on the display
  gui_load_scr(scr_loading);
}

void gui_loading_show_reconnect_btn(on_wifi_reconnect btn_clicked) {
  if (!lvgl_lock(-1, "gui_loading_show_reconnect_btn")) {
    ESP_LOGE(TAG, "Failed to acquire lock for showing loading buttons");
    return;
  }

  // create a button container
  btn_cont = lv_obj_create(scr_loading);
  lv_obj_set_flex_grow(btn_cont, 1);
  lv_obj_set_width(btn_cont, LV_PCT(100));
  lv_obj_set_style_border_width(btn_cont, 0, LV_PART_MAIN);

  // add reconnect wifi button
  lv_obj_t *btn_wifi = lv_btn_create(btn_cont);
  lv_obj_set_size(btn_wifi, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
  lv_obj_align(btn_wifi, LV_ALIGN_CENTER, 0, 0);
  lv_obj_add_event_cb(btn_wifi, btn_clicked, LV_EVENT_CLICKED, NULL);

  // button label
  lv_obj_t *lbl_wifi = lv_label_create(btn_wifi);
  lv_label_set_text(lbl_wifi, "Reconnect WiFi");
  lv_obj_center(lbl_wifi);

  lvgl_unlock();
}

void gui_loading_add_log(const char *msg) {
  ESP_LOGI(TAG, "[LOG]: %s", msg);

  if (lbl_logs == NULL) {
    return;
  }

  // Shift all logs to the right
  for (int i = MAX_LOGS - 1; i > 0; i--) {
    strncpy(logs[i], logs[i - 1], MAX_LOG_SIZE);
    logs[i][MAX_LOG_SIZE - 1] = '\0';  // Ensure null-termination
  }

  // Add the new log to the first position
  strncpy(logs[0], msg, MAX_LOG_SIZE);
  logs[0][MAX_LOG_SIZE - 1] = '\0';

  // reset logs string
  log_str[0] = '\0';

  // Join logs to one string
  for (int i = MAX_LOGS - 1; i >= 0; i--) {
    // don't print out empty strings
    if (strcmp(logs[i], "") == 0) {
      continue;
    }

    strncat(log_str, logs[i], sizeof(log_str) - strlen(log_str) - 1);
    strncat(log_str, "\n", sizeof(log_str) - strlen(log_str) - 1);
  }

  // Update LVGL label
  if (lvgl_lock(-1, "gui_loading_add_log")) {
    lv_label_set_text(lbl_logs, log_str);
    lv_obj_scroll_to_y(logs_cont, LV_COORD_MAX, LV_ANIM_ON);
    lvgl_unlock();
  }
}
