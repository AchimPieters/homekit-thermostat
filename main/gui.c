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

// GUI objects
// ------------------------
// Flexbox containers
static lv_obj_t *main_cont;
static lv_obj_t *data_cont;
static lv_obj_t *btns_cont;

// Font styles
static lv_style_t style_font32;
static lv_style_t style_font48;
static lv_style_t style_font26;

// Labels
static lv_obj_t *label_targ_temp;
static lv_obj_t *label_curr_temp;
static lv_obj_t *time_label;
static lv_obj_t *label_date;
static lv_obj_t *label_btn_incr;
static lv_obj_t *label_btn_decr;

// Buttons
static lv_obj_t *btn_incr;
static lv_obj_t *btn_decr;
// Button pressed callback function
void (*btn_pressed_callback)(enum ButtonType type);

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
static void lvgl_touch_cb(lv_indev_drv_t *drv, lv_indev_data_t *data) {
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

static void on_btn_pressed(lv_event_t *e) {
  lv_obj_t * btn = lv_event_get_target(e);
  enum ButtonType btn_type = (enum ButtonType) lv_obj_get_user_data(btn);

  // If the callback function is defined, call it
  if (btn_pressed_callback != NULL) {
    btn_pressed_callback(btn_type);
  }
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

  // Font styles
  lv_style_init(&style_font26);
  lv_style_set_text_font(&style_font26, &lv_font_montserrat_26);
  lv_style_init(&style_font32);
  lv_style_set_text_font(&style_font32, &lv_font_montserrat_32);
  lv_style_init(&style_font48);
  lv_style_set_text_font(&style_font48, &lv_font_montserrat_48);
}

static void create_btn(lv_obj_t *btn, lv_obj_t *lbl, enum ButtonType btn_type) {
  btn = lv_btn_create(btns_cont);
  lv_obj_add_event_cb(btn, on_btn_pressed, LV_EVENT_CLICKED, NULL);
  lv_obj_set_size(btn, LV_PCT(100), LV_SIZE_CONTENT);
  lv_obj_set_flex_grow(btn, 1);
  lv_obj_set_user_data(btn, (void *) btn_type);

  lbl = lv_label_create(btn);
  lv_label_set_text(lbl, btn_type == BUTTON_INCREASE ? "+" : "-");
  lv_obj_center(lbl);
  lv_obj_add_style(lbl, &style_font32, LV_PART_MAIN);
}

void gui_render() {
  ESP_LOGI(TAG, "Rendering UI");

  // rotate display by 90°
  lv_disp_set_rotation(disp, LV_DISP_ROT_90);

  // create main flexbox row container
  main_cont = lv_obj_create(lv_scr_act());
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
  // this label will grow to take all the available white space
  label_curr_temp = lv_label_create(data_cont);
  lv_label_set_recolor(label_curr_temp, true);
  lv_obj_add_style(label_curr_temp, &style_font26, LV_PART_MAIN);
  lv_obj_set_flex_grow(label_curr_temp, 1);

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

  create_btn(btn_incr, label_btn_incr, BUTTON_INCREASE);
  create_btn(btn_decr, label_btn_decr, BUTTON_DECREASE);
}

// GUI Handlers
void gui_on_btn_pressed_cb(void (*cb)(enum ButtonType type)) {
  btn_pressed_callback = cb;
}

void gui_set_temp(const char *current, const char *target, const char *thermostat_status) {
  // target temperature
  char text[30];
  sprintf(text, "#0096FF %s°C#", target);
  lv_label_set_text(label_targ_temp, text);
  
  // current temperature and status
  sprintf(text, "#FFBF00 %s°C#\n#000 %s#", current, thermostat_status);
  lv_label_set_text(label_curr_temp, text);
}

void gui_set_datetime(const char *date, const char *time) {
  lv_label_set_text(label_date, date);
  lv_label_set_text(time_label, time);
}