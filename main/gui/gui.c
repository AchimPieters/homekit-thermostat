#include <lvgl.h>
#include <stdio.h>
#include <esp_lcd_ili9341.h>
#include <esp_lcd_panel_io.h>
#include <esp_lcd_panel_ops.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>
#include <esp_log.h>
#include <esp_timer.h>
#include <lv_qrcode.h>
#include "gui.h"
#include "fonts.h"
#include "../hw/lcd.h"
#include "../tasks/tasks.h"

lv_disp_draw_buf_t lvgl_disp_buf;  // contains internal graphic buffer(s) called draw buffer(s)
lv_disp_drv_t lvgl_disp_drv;       // contains callback functions

static const char *TAG = "GUI";

lv_obj_t *gui_active_scr = NULL;

// LVGL stuff
// ------------------------
static SemaphoreHandle_t lvgl_mux = NULL;
static lv_disp_t *disp = NULL;

// Tick interface for LVGL (using esp_timer to generate 2ms periodic event)
static const esp_timer_create_args_t lvgl_tick_timer_args = {
  .callback = &lvgl_increase_tick,
  .name = "lvgl_tick"
};
static esp_timer_handle_t lvgl_tick_timer = NULL;
static lv_color_t *buf1, *buf2;
static lv_indev_drv_t indev_drv;

bool lvgl_notify_flush_ready(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_io_event_data_t *edata, void *user_ctx) {
  lv_disp_drv_t *disp_driver = (lv_disp_drv_t *) user_ctx;
  lv_disp_flush_ready(disp_driver);
  return false;
}

void lvgl_flush_cb(lv_disp_drv_t *display_drv, const lv_area_t *area, lv_color_t *color_map) {
  esp_lcd_panel_handle_t lcd_panel = (esp_lcd_panel_handle_t) display_drv->user_data;
  // copy a buffer's content to a specific area of the display
  esp_lcd_panel_draw_bitmap(lcd_panel, area->x1, area->y1, area->x2 + 1, area->y2 + 1, color_map);
}

void lvgl_increase_tick(void *arg) {
  // Tell LVGL how many milliseconds has elapsed
  lv_tick_inc(LVGL_TICK_PERIOD_MS);
}

bool lvgl_lock(int timeout_ms, char* msg) {
  ESP_LOGD(TAG, "LVGL Lock [%s]", msg);
  // Convert timeout in milliseconds to FreeRTOS ticks
  // If `timeout_ms` is set to -1, the program will block until the condition is met
  const TickType_t timeout_ticks = (timeout_ms == -1) ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms);
  return xSemaphoreTakeRecursive(lvgl_mux, timeout_ticks) == pdTRUE;
}

void lvgl_unlock(void) {
  ESP_LOGD(TAG, "LVGL Unlock");
  xSemaphoreGiveRecursive(lvgl_mux);
}

// Display touch callback
static void lvgl_touch_cb(lv_indev_drv_t *drv, lv_indev_data_t *data) {
  uint16_t touchpad_x[1] = {0};
  uint16_t touchpad_y[1] = {0};
  uint8_t touchpad_cnt = 0;

  esp_lcd_touch_read_data(drv->user_data);

  bool touchpad_pressed = esp_lcd_touch_get_coordinates(drv->user_data, touchpad_x, touchpad_y, NULL, &touchpad_cnt, 1);
  if (touchpad_pressed) {
    ESP_LOGD(TAG, "Touched at point x: %d, y: %d\n", touchpad_x[0], touchpad_y[0]);
  }

  if (touchpad_pressed && touchpad_cnt > 0) {
    data->point.x = touchpad_x[0];
    data->point.y = touchpad_y[0];
    data->state = LV_INDEV_STATE_PRESSED;
  } else {
    data->state = LV_INDEV_STATE_RELEASED;
  }
}

void gui_init(void) {
  ESP_LOGI(TAG, "Initialize LVGL library");
  lv_init();

  // alloc draw buffers used by LVGL
  // it's recommended to choose the size of the draw buffer(s) to be at least 1/10 screen sized
  buf1 = heap_caps_malloc(LCD_HORIZONTAL_RES * 50 * sizeof(lv_color_t), MALLOC_CAP_DMA);
  assert(buf1);
  buf2 = heap_caps_malloc(LCD_HORIZONTAL_RES * 50 * sizeof(lv_color_t), MALLOC_CAP_DMA);
  assert(buf2);

  // initialize LVGL draw buffers
  lv_disp_draw_buf_init(&lvgl_disp_buf, buf1, buf2, LCD_HORIZONTAL_RES * 50); // 50 - number of lines in the frame buffer

  ESP_LOGI(TAG, "Register display driver to LVGL");
  lv_disp_drv_init(&lvgl_disp_drv);
  lvgl_disp_drv.hor_res = LCD_HORIZONTAL_RES;
  lvgl_disp_drv.ver_res = LCD_VERTICAL_RES;
  lvgl_disp_drv.flush_cb = lvgl_flush_cb;
  lvgl_disp_drv.draw_buf = &lvgl_disp_buf;
  lvgl_disp_drv.user_data = lcd_panel_handle;
  disp = lv_disp_drv_register(&lvgl_disp_drv);

  ESP_LOGI(TAG, "Install LVGL tick timer");
  ESP_ERROR_CHECK(esp_timer_create(&lvgl_tick_timer_args, &lvgl_tick_timer));
  ESP_ERROR_CHECK(esp_timer_start_periodic(lvgl_tick_timer, LVGL_TICK_PERIOD_MS * 1000));

  // LVGL + TOUCH
  lv_indev_drv_init(&indev_drv);
  indev_drv.type = LV_INDEV_TYPE_POINTER;
  indev_drv.disp = disp;
  indev_drv.read_cb = lvgl_touch_cb;
  indev_drv.user_data = lcd_touch_handle;
  lv_indev_drv_register(&indev_drv);

  lvgl_mux = xSemaphoreCreateRecursiveMutex();
  assert(lvgl_mux);

  // Font styles
  gui_init_fonts();

  // rotate display by 90°
  lv_disp_set_rotation(disp, LV_DISP_ROT_90);

  // LVGL timer task
  xTaskCreate(task_lvgl, "LVGL timer", LVGL_TASK_STACK_SIZE, NULL, LVGL_TASK_PRIORITY, NULL);
}

void gui_load_scr(lv_obj_t *scr) {
  // Lock the mutex due to the LVGL APIs are not thread-safe
  if (lvgl_lock(-1, "gui_load_scr")) {
    lv_scr_load(scr);
    gui_active_scr = scr;
    lvgl_unlock();
  }
}
