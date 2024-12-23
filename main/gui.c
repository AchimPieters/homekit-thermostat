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
#include "gui.h"
#include "lcd.h"

lv_disp_draw_buf_t lvgl_disp_buf;  // contains internal graphic buffer(s) called draw buffer(s)
lv_disp_drv_t lvgl_disp_drv;       // contains callback functions

static const char *TAG = "GUI";
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
// UI objects
static lv_obj_t *meter;
static lv_obj_t *btn;

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

bool lvgl_lock(int timeout_ms) {
  // Convert timeout in milliseconds to FreeRTOS ticks
  // If `timeout_ms` is set to -1, the program will block until the condition is met
  const TickType_t timeout_ticks = (timeout_ms == -1) ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms);
  return xSemaphoreTakeRecursive(lvgl_mux, timeout_ticks) == pdTRUE;
}

void lvgl_unlock(void) {
  xSemaphoreGiveRecursive(lvgl_mux);
}

void lvgl_timer_task(void *arg) {
  ESP_LOGI(TAG, "Starting LVGL timer task");
  uint32_t task_delay_ms = LVGL_TASK_MAX_DELAY_MS;
  
  while (1) {
    // Lock the mutex due to the LVGL APIs are not thread-safe
    if (lvgl_lock(-1)) {
      task_delay_ms = lv_timer_handler();
      // Release the mutex
      lvgl_unlock();
    }
    if (task_delay_ms > LVGL_TASK_MAX_DELAY_MS) {
      task_delay_ms = LVGL_TASK_MAX_DELAY_MS;
    } else if (task_delay_ms < LVGL_TASK_MIN_DELAY_MS) {
      task_delay_ms = LVGL_TASK_MIN_DELAY_MS;
    }
    vTaskDelay(pdMS_TO_TICKS(task_delay_ms));
  }
}

// Display touch callback
void lvgl_touch_cb(lv_indev_drv_t *drv, lv_indev_data_t *data) {
  // TODO(improvement): use interrupts instead of polling
  //  note - check if these readings are done by LVGL or by the driver itself, who's sending so many read requests?
  // printf("touch callback\n");

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

static void set_value(void *indic, int32_t v) {
  lv_meter_set_indicator_end_value(meter, indic, v);
}

static void btn_cb(lv_event_t *e) {
  printf("Button handler called\n");

  // TODO: implement code
}

void gui_init(void) {
  ESP_LOGI(TAG, "Initialize LVGL library");
  lv_init();

  // alloc draw buffers used by LVGL
  // it's recommended to choose the size of the draw buffer(s) to be at least 1/10 screen sized
  buf1 = heap_caps_malloc(LCD_HORIZONTAL_RES * 20 * sizeof(lv_color_t), MALLOC_CAP_DMA);
  assert(buf1);
  buf2 = heap_caps_malloc(LCD_HORIZONTAL_RES * 20 * sizeof(lv_color_t), MALLOC_CAP_DMA);
  assert(buf2);

  // initialize LVGL draw buffers
  lv_disp_draw_buf_init(&lvgl_disp_buf, buf1, buf2, LCD_HORIZONTAL_RES * 20); // TODO: what is this 20?

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
}

void gui_render() {
  ESP_LOGI(TAG, "Rendering UI");

  // display is turned by 90°
  lv_disp_set_rotation(disp, LV_DISP_ROT_90);

  lv_obj_t *scr = lv_disp_get_scr_act(disp);
  meter = lv_meter_create(scr);
  lv_obj_center(meter);
  lv_obj_set_size(meter, 200, 200);

  /*Add a scale first*/
  lv_meter_scale_t *scale = lv_meter_add_scale(meter);
  lv_meter_set_scale_ticks(meter, scale, 41, 2, 10, lv_palette_main(LV_PALETTE_GREY));
  lv_meter_set_scale_major_ticks(meter, scale, 8, 4, 15, lv_color_black(), 10);

  lv_meter_indicator_t *indic;

  /*Add a blue arc to the start*/
  indic = lv_meter_add_arc(meter, scale, 3, lv_palette_main(LV_PALETTE_BLUE), 0);
  lv_meter_set_indicator_start_value(meter, indic, 0);
  lv_meter_set_indicator_end_value(meter, indic, 20);

  /*Make the tick lines blue at the start of the scale*/
  indic = lv_meter_add_scale_lines(meter, scale, lv_palette_main(LV_PALETTE_BLUE), lv_palette_main(LV_PALETTE_BLUE), false, 0);
  lv_meter_set_indicator_start_value(meter, indic, 0);
  lv_meter_set_indicator_end_value(meter, indic, 20);

  /*Add a red arc to the end*/
  indic = lv_meter_add_arc(meter, scale, 3, lv_palette_main(LV_PALETTE_RED), 0);
  lv_meter_set_indicator_start_value(meter, indic, 80);
  lv_meter_set_indicator_end_value(meter, indic, 100);

  /*Make the tick lines red at the end of the scale*/
  indic = lv_meter_add_scale_lines(meter, scale, lv_palette_main(LV_PALETTE_RED), lv_palette_main(LV_PALETTE_RED), false, 0);
  lv_meter_set_indicator_start_value(meter, indic, 80);
  lv_meter_set_indicator_end_value(meter, indic, 100);

  /*Add a needle line indicator*/
  indic = lv_meter_add_needle_line(meter, scale, 4, lv_palette_main(LV_PALETTE_GREY), -10);

  btn = lv_btn_create(scr);
  lv_obj_t *lbl = lv_label_create(btn);
  lv_label_set_text_static(lbl, LV_SYMBOL_REFRESH " ROTATE");
  lv_obj_align(btn, LV_ALIGN_BOTTOM_LEFT, 30, -30);
  /*Button event*/
  lv_obj_add_event_cb(btn, btn_cb, LV_EVENT_CLICKED, disp);

  /*Create an animation to set the value*/
  lv_anim_t a;
  lv_anim_init(&a);
  lv_anim_set_exec_cb(&a, set_value);
  lv_anim_set_var(&a, indic);
  lv_anim_set_values(&a, 0, 100);
  lv_anim_set_time(&a, 2000);
  lv_anim_set_repeat_delay(&a, 100);
  lv_anim_set_playback_time(&a, 500);
  lv_anim_set_playback_delay(&a, 100);
  lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
  lv_anim_start(&a);
}
