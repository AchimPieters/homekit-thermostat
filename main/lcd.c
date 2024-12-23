#include "lcd.h"

#include <driver/gpio.h>
#include <driver/spi_master.h>
#include <esp_err.h>
#include <esp_lcd_ili9341.h>
#include <esp_lcd_panel_io.h>
#include <esp_lcd_panel_ops.h>
#include <esp_lcd_panel_vendor.h>
#include <esp_lcd_touch_xpt2046.h>
#include <esp_log.h>
#include <lvgl.h>
#include "gui.h"

esp_lcd_panel_handle_t lcd_panel_handle = NULL;
esp_lcd_touch_handle_t lcd_touch_handle = NULL;

static const char *TAG = "LCD";
static spi_bus_config_t buscfg = {
    .sclk_io_num = PIN_NUM_SCLK,
    .mosi_io_num = PIN_NUM_MOSI,
    .miso_io_num = PIN_NUM_MISO,
    .quadwp_io_num = -1,
    .quadhd_io_num = -1,
    .max_transfer_sz = LCD_HORIZONTAL_RES * 80 * sizeof(uint16_t),
};
static gpio_config_t bk_gpio_config = {
    .mode = GPIO_MODE_OUTPUT,
    .pin_bit_mask = 1ULL << PIN_NUM_BK_LIGHT,
};
// LCD 
static esp_lcd_panel_io_handle_t io_handle = NULL;
static esp_lcd_panel_io_spi_config_t io_config = {
    .dc_gpio_num = PIN_NUM_LCD_DC,
    .cs_gpio_num = PIN_NUM_LCD_CS,
    .pclk_hz = LCD_PIXEL_CLOCK_HZ,
    .lcd_cmd_bits = LCD_CMD_BITS,
    .lcd_param_bits = LCD_PARAM_BITS,
    .spi_mode = 0,
    .trans_queue_depth = 10,
    // .on_color_trans_done
    // .user_ctx
};
static esp_lcd_panel_dev_config_t panel_config = {
    .reset_gpio_num = PIN_NUM_LCD_RST,
    .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_BGR,
    .bits_per_pixel = 16,
};
// TOUCH
static esp_lcd_panel_io_handle_t tp_io_handle = NULL;
static esp_lcd_panel_io_spi_config_t tp_io_config = ESP_LCD_TOUCH_IO_SPI_XPT2046_CONFIG(PIN_NUM_TOUCH_CS);
static esp_lcd_touch_config_t tp_config = {
    .x_max = LCD_HORIZONTAL_RES,
    .y_max = LCD_VERTICAL_RES,
    .rst_gpio_num = -1,
    .int_gpio_num = -1,  // TODO: would be nice to actually use interrupts instead of polling
    .flags = {
        .swap_xy = false,
        .mirror_x = false,
        .mirror_y = false,
    },
};

void lcd_init() {
  ESP_LOGI(TAG, "Initialize SPI bus");
  ESP_ERROR_CHECK(spi_bus_initialize(LCD_HOST, &buscfg, SPI_DMA_CH_AUTO));

  ESP_LOGI(TAG, "Configure LCD backlight pin");
  ESP_ERROR_CHECK(gpio_config(&bk_gpio_config));

  ESP_LOGI(TAG, "Install panel IO");
  io_config.on_color_trans_done = lvgl_notify_flush_ready;
  io_config.user_ctx = &lvgl_disp_drv;

  // Attach the LCD to the SPI bus
  ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t) LCD_HOST, &io_config, &io_handle));

  ESP_LOGI(TAG, "Install ILI9341 panel driver");
  ESP_ERROR_CHECK(esp_lcd_new_panel_ili9341(io_handle, &panel_config, &lcd_panel_handle));
  ESP_ERROR_CHECK(esp_lcd_panel_reset(lcd_panel_handle));
  ESP_ERROR_CHECK(esp_lcd_panel_init(lcd_panel_handle));
  // display is turned by 90° so we need to swap x and y
  ESP_ERROR_CHECK(esp_lcd_panel_swap_xy(lcd_panel_handle, true));
  ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(lcd_panel_handle, true));

  // Attach the TOUCH to the SPI bus
  ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t) LCD_HOST, &tp_io_config, &tp_io_handle));

  ESP_LOGI(TAG, "Initialize touch controller XPT2046");
  ESP_ERROR_CHECK(esp_lcd_touch_new_spi_xpt2046(tp_io_handle, &tp_config, &lcd_touch_handle));

  ESP_LOGI(TAG, "Turn on LCD backlight");
  gpio_set_level(PIN_NUM_BK_LIGHT, LCD_BK_LIGHT_ON_LEVEL);
}