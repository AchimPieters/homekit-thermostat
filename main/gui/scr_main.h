#include <lvgl.h>
#include "../homekit.h"

void gui_main_scr(void);

typedef enum {
  BUTTON_INCREASE,
  BUTTON_DECREASE
} ButtonType;

typedef void (*temp_button_callback)(ButtonType type);

// GUI handlers
void gui_on_btn_pressed_cb(temp_button_callback);
void gui_set_target_temp(float target);
void gui_set_curr_temp(float current);
void gui_set_thermostat_status(ThermostatStatus thermostat_status);
void gui_set_datetime(const char *date, const char *time);
