#include "scr_loading.h"

#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <lvgl.h>

#include "gui.h"

#define MAX_LOG_SIZE 1024  // Limit log size to prevent memory overflow

static const char *TAG = "LOADING SCREEN";

static lv_obj_t *scr_loading;
static lv_obj_t *logs_cont;
static lv_obj_t *lbl_logs;
static lv_obj_t *btn_cont;

static char *logs = NULL;

void gui_loading_scr() {
  ESP_LOGI(TAG, "Rendering");

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

  size_t new_log_len = strlen(msg);
  size_t current_len = logs ? strlen(logs) : 0;
  size_t new_size = current_len + new_log_len + 2;  // +1 for newline, +1 for '\0'

  // Prevent excessive growth
  if (new_size > MAX_LOG_SIZE) {
    size_t remove_size = new_size - MAX_LOG_SIZE;
    if (remove_size > current_len) {
      remove_size = current_len;  // Prevent underflow
    }
    memmove(logs, logs + remove_size, current_len - remove_size + 1);  // Move remaining data
    current_len -= remove_size;
    new_size = current_len + new_log_len + 2;
  }

  // Expand buffer
  char *temp = realloc(logs, new_size);
  if (!temp) {
    ESP_LOGE(TAG, "Memory allocation failed");
    return;
  }
  logs = temp;

  // Append the new log
  strcpy(logs + current_len, msg);
  strcat(logs, "\n");

  // Update LVGL label
  if (lvgl_lock(-1, "gui_loading_add_log")) {
    lv_label_set_text(lbl_logs, logs);
    lv_obj_scroll_to_y(logs_cont, LV_COORD_MAX, LV_ANIM_ON);
    lvgl_unlock();
  }
}
