#include <lvgl.h>
#include <stdio.h>
#include <esp_log.h>
#include <lv_qrcode.h>
#include "fonts.h"
#include "gui.h"

static const char *TAG = "WIFI SCREEN";

static lv_obj_t *scr_wifi_setup;

void gui_wifi_scr(char *qr_data, size_t qr_data_len) {
  ESP_LOGI(TAG, "Rendering");

  // create main flexbox row container
  scr_wifi_setup = lv_obj_create(NULL);
  lv_obj_set_size(scr_wifi_setup, LV_PCT(100), LV_PCT(100));
  lv_obj_set_flex_flow(scr_wifi_setup, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_style_pad_all(scr_wifi_setup, 10, LV_PART_MAIN);
  lv_obj_set_style_border_width(scr_wifi_setup, 0, LV_PART_MAIN);
  lv_obj_set_scrollbar_mode(scr_wifi_setup, LV_SCROLLBAR_MODE_OFF);

  // add title
  lv_obj_t *lbl_title = lv_label_create(scr_wifi_setup);
  lv_label_set_recolor(lbl_title, true);
  lv_label_set_text(lbl_title, "WiFi Setup");
  lv_obj_add_style(lbl_title, &style_font26, LV_PART_MAIN);

  // add description
  lv_obj_t *lbl_descr = lv_label_create(scr_wifi_setup);
  lv_label_set_recolor(lbl_descr, true);
  lv_label_set_text(lbl_descr, "Download the ESP BLE Provisioning app to configure the WiFi network for the thermostat first.");
  lv_label_set_long_mode(lbl_descr, LV_LABEL_LONG_WRAP);
  lv_obj_set_width(lbl_descr, lv_pct(100));

  // add QR code container
  lv_obj_t *qr_container = lv_obj_create(scr_wifi_setup);
  lv_obj_set_width(qr_container, lv_pct(100));
  lv_obj_set_flex_align(qr_container, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_border_width(qr_container, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_all(qr_container, 0, LV_PART_MAIN);

  // add QR code (lv_canvas)
  lv_obj_t *qr = lv_qrcode_create(qr_container, 130, lv_color_black(), lv_color_white());
  lv_qrcode_update(qr, qr_data, qr_data_len);

  // show screen on the display
  gui_load_scr(scr_wifi_setup);
}
