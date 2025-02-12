#include "scr_main.h"

#include <esp_log.h>
#include <lvgl.h>
#include <stdio.h>

#include "fonts.h"
#include "gui.h"

static const char *TAG = "MAIN SCREEN";

// GUI objects
// ------------------------
// Flexbox containers
static lv_obj_t *main_cont;
static lv_obj_t *data_cont;
static lv_obj_t *btns_cont;

// Labels
static lv_obj_t *label_targ_temp;
static lv_obj_t *label_curr_temp;
static lv_obj_t *label_thermostat_status;
static lv_obj_t *time_label;
static lv_obj_t *label_date;
static lv_obj_t *label_btn_incr;
static lv_obj_t *label_btn_decr;

// Buttons
static lv_obj_t *btn_incr;
static lv_obj_t *btn_decr;
static lv_style_t style_btn_disabled;

// Button pressed callback function
temp_button_callback btn_pressed_callback;

static void on_btn_pressed(lv_event_t *e) {
  lv_obj_t *btn = lv_event_get_target(e);
  ButtonType btn_type = (ButtonType)lv_obj_get_user_data(btn);

  // If the callback function is defined, call it
  if (btn_pressed_callback != NULL) {
    btn_pressed_callback(btn_type);
  }
}

static void create_btn(lv_obj_t **btn, lv_obj_t *lbl, ButtonType btn_type) {
  *btn = lv_btn_create(btns_cont);
  lv_obj_add_event_cb(*btn, on_btn_pressed, LV_EVENT_CLICKED, NULL);
  lv_obj_set_size(*btn, LV_PCT(100), LV_SIZE_CONTENT);
  lv_obj_set_flex_grow(*btn, 1);
  lv_obj_set_user_data(*btn, (void *)btn_type);
  lv_obj_add_style(*btn, &style_btn_disabled, LV_STATE_DISABLED);

  lbl = lv_label_create(*btn);
  lv_label_set_text(lbl, btn_type == BUTTON_INCREASE ? "+" : "-");
  lv_obj_center(lbl);
  lv_obj_add_style(lbl, &style_font32, LV_PART_MAIN);
}

void gui_main_scr() {
  ESP_LOGI(TAG, "Rendering");

  // create main flexbox row container
  main_cont = lv_obj_create(NULL);
  lv_obj_set_size(main_cont, LV_PCT(100), LV_PCT(100));
  lv_obj_align(main_cont, LV_ALIGN_CENTER, 0, 0);
  lv_obj_set_flex_flow(main_cont, LV_FLEX_FLOW_ROW);
  lv_obj_set_style_pad_all(main_cont, 10, LV_PART_MAIN);

  // create data flexbox container
  // it will take 3/5 of the screen
  // now we're in the left part of the screen
  data_cont = lv_obj_create(main_cont);
  lv_obj_set_height(data_cont, LV_PCT(100));
  lv_obj_set_flex_grow(data_cont, 3);
  lv_obj_set_flex_flow(data_cont, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_style_border_width(data_cont, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_all(data_cont, 0, LV_PART_MAIN);

  // add a target temperature label at the top
  label_targ_temp = lv_label_create(data_cont);
  lv_label_set_recolor(label_targ_temp, true);
  lv_obj_add_style(label_targ_temp, &style_font48, LV_PART_MAIN);

  // underneath, add a current temperature label
  label_curr_temp = lv_label_create(data_cont);
  lv_label_set_recolor(label_curr_temp, true);
  lv_obj_add_style(label_curr_temp, &style_font26, LV_PART_MAIN);

  // next, add a thermostat status label
  // this label will grow to take all the available white space
  label_thermostat_status = lv_label_create(data_cont);
  lv_label_set_recolor(label_thermostat_status, true);
  lv_obj_add_style(label_thermostat_status, &style_font26, LV_PART_MAIN);
  lv_obj_set_flex_grow(label_thermostat_status, 1);

  // at the bottom, add a time label
  time_label = lv_label_create(data_cont);
  lv_label_set_recolor(time_label, true);
  lv_obj_add_style(time_label, &style_font32, LV_PART_MAIN);

  // add a date label
  label_date = lv_label_create(data_cont);
  lv_label_set_recolor(label_date, true);

  // create buttons container
  // it will take 2/5 of the screen
  // now we're in the right part of the screen
  btns_cont = lv_obj_create(main_cont);
  lv_obj_set_height(btns_cont, LV_PCT(100));
  lv_obj_set_flex_grow(btns_cont, 2);
  lv_obj_set_flex_flow(btns_cont, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_style_border_width(btns_cont, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_all(btns_cont, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_row(btns_cont, 20, LV_PART_MAIN);

  // Init disabled button styles
  lv_style_init(&style_btn_disabled);
  lv_style_set_bg_color(&style_btn_disabled, lv_palette_darken(LV_PALETTE_GREY, 3));  // Gray out the button
  lv_style_set_border_color(&style_btn_disabled, lv_palette_main(LV_PALETTE_GREY));   // Dim the border

  create_btn(&btn_incr, label_btn_incr, BUTTON_INCREASE);
  create_btn(&btn_decr, label_btn_decr, BUTTON_DECREASE);

  gui_load_scr(main_cont);
}

// GUI Handlers
void gui_on_btn_pressed_cb(temp_button_callback cb) {
  btn_pressed_callback = cb;
}

static void gui_enable_btns(bool enable) {
  if (enable) {
    lv_obj_clear_state(btn_incr, LV_STATE_DISABLED);
    lv_obj_clear_state(btn_decr, LV_STATE_DISABLED);
  } else {
    lv_obj_add_state(btn_incr, LV_STATE_DISABLED);
    lv_obj_add_state(btn_decr, LV_STATE_DISABLED);
  }
}

void gui_set_target_temp(float target_temp) {
   if (label_targ_temp == NULL) {
    return;
  }

  char text[20];
  sprintf(text, "#0096FF %.1f°C#", target_temp);
  lv_label_set_text(label_targ_temp, text);
}

void gui_set_curr_temp(float current) {
  if (label_curr_temp == NULL) {
    return;
  }

  char text[20];
  sprintf(text, "#ffa500 %.1f°C#", current);
  lv_label_set_text(label_curr_temp, text);
}

void gui_set_thermostat_status(ThermostatStatus thermostat_status) {
  if (label_thermostat_status == NULL) {
    return;
  }

  switch (thermostat_status) {
    case THERMOSTAT_HEAT:
      lv_label_set_text(label_thermostat_status, "#ffa500 heating #");
      gui_enable_btns(true);
      break;
    case _THERMOSTAT_IDLE:
      lv_label_set_text(label_thermostat_status, "#50C878 idle #");
      gui_enable_btns(true);
      break;
    case THERMOSTAT_OFF:
    default:
      lv_label_set_text(label_thermostat_status, "off");
      gui_enable_btns(false);
      break;
  }
}

void gui_set_datetime(const char *date, const char *time) {
  if (label_date == NULL || time_label == NULL) {
    return;
  }

  lv_label_set_text(label_date, date);
  lv_label_set_text(time_label, time);
}

// TODO: add locks to gui handlers
