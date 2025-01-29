#include <lvgl.h>

// Font styles
lv_style_t style_font32;
lv_style_t style_font48;
lv_style_t style_font26;

void gui_init_fonts() {
  lv_style_init(&style_font26);
  lv_style_set_text_font(&style_font26, &lv_font_montserrat_26);
  lv_style_init(&style_font32);
  lv_style_set_text_font(&style_font32, &lv_font_montserrat_32);
  lv_style_init(&style_font48);
  lv_style_set_text_font(&style_font48, &lv_font_montserrat_48);
}
