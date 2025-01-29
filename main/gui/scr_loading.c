#include <lvgl.h>
#include <esp_log.h>

static const char *TAG = "Loading screen";

lv_obj_t *scr_loading;

static lv_obj_t *lbl_logs;

void gui_loading_scr(void (*on_wifi_reconnect)(void)) {
  ESP_LOGI(TAG, "Rendering screen");

  // create main flexbox row container
  scr_loading = lv_obj_create(NULL);
  lv_obj_set_size(scr_loading, LV_PCT(100), LV_PCT(100));
  lv_obj_set_style_pad_all(scr_loading, 10, LV_PART_MAIN);
  lv_obj_set_flex_flow(scr_loading, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_style_border_width(scr_loading, 0, LV_PART_MAIN);

  // add logs label
  lbl_logs = lv_label_create(scr_loading);
  lv_label_set_recolor(lbl_logs, true);
  lv_label_set_text(lbl_logs, "Initializing...\nConnecting to WiFi...\nFetching current time...\n#ff0000 Failed to connect to HomeKit#");
  lv_obj_set_flex_grow(lbl_logs, 1);

  // add reconnect wifi button
  lv_obj_t *btn_wifi = lv_btn_create(scr_loading);
  lv_obj_set_size(btn_wifi, LV_PCT(100), LV_SIZE_CONTENT);
  lv_obj_t *lbl_wifi = lv_label_create(btn_wifi);
  lv_obj_set_style_pad_top(btn_wifi, 20, LV_PART_MAIN);    // Top padding
  lv_obj_set_style_pad_bottom(btn_wifi, 20, LV_PART_MAIN); // Bottom padding
  lv_obj_align(btn_wifi, LV_ALIGN_CENTER, 0, 0);
  lv_obj_add_event_cb(btn_wifi, on_wifi_reconnect, LV_EVENT_CLICKED, NULL);

  lv_label_set_text(lbl_wifi, "Reconnect WiFi");
  lv_obj_center(lbl_wifi);
}
