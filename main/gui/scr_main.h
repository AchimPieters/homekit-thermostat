#include <lvgl.h>

extern lv_obj_t *scr_loading;

void gui_main_scr(void);

enum ButtonType {
  BUTTON_INCREASE,
  BUTTON_DECREASE
};

// GUI handlers
void gui_on_btn_pressed_cb(void (*cb)(enum ButtonType type));
void gui_set_target_temp(const char *target);
void gui_set_curr_temp(const char *current);
void gui_set_thermostat_status(const char *thermostat_status);
void gui_set_datetime(const char *date, const char *time);
