#include <lvgl.h>
#include <esp_lcd_ili9341.h>

#define LVGL_TICK_PERIOD_MS 2
#define LVGL_TASK_MAX_DELAY_MS 500
#define LVGL_TASK_MIN_DELAY_MS 1
#define LVGL_TASK_STACK_SIZE (4 * 1024)
#define LVGL_TASK_PRIORITY 2

extern lv_disp_draw_buf_t lvgl_disp_buf;  // contains internal graphic buffer(s) called draw buffer(s)
extern lv_disp_drv_t lvgl_disp_drv;

bool lvgl_notify_flush_ready(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_io_event_data_t *edata, void *user_ctx);
void lvgl_flush_cb(lv_disp_drv_t *display_drv, const lv_area_t *area, lv_color_t *color_map);
void lvgl_increase_tick(void *arg);
bool lvgl_lock(int timeout_ms);
void lvgl_unlock(void);

void gui_init(void);
void gui_load_scr(lv_obj_t *scr);
