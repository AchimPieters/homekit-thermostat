
#include <stdio.h>
#include <esp_wifi.h>
#include <esp_log.h>
#include <esp_system.h>
#include <string.h>
#include "err.h"

void wifi_restart() {
  ESP_LOGI("INFORMATION", "Restarting WiFi...");
  esp_wifi_stop();
  esp_wifi_start();
}

void wifi_handle_err(esp_err_t err) {
  switch (err) {
    case ESP_ERR_WIFI_NOT_STARTED:
    case ESP_ERR_WIFI_CONN:
      wifi_restart();
      break;
    default:
      handle_err(err);
      break;
  }
}

void wifi_connect(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
  // Cast the argument back to a on_wifi_ready callback
  void (*on_wifi_ready)(void) = (void (*)(void))arg;

  if (event_base == WIFI_EVENT && (event_id == WIFI_EVENT_STA_START || event_id == WIFI_EVENT_STA_DISCONNECTED)) {
    ESP_LOGI("INFORMATION", "Connecting to WiFi...");
    esp_wifi_connect();
  } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
    ESP_LOGI("INFORMATION", "WiFi connected, IP obtained");
    on_wifi_ready();
  }
}

void wifi_init(void (*on_wifi_ready)(void)) {
  wifi_handle_err(esp_netif_init());
  wifi_handle_err(esp_event_loop_create_default());
  esp_netif_create_default_wifi_sta();

  wifi_handle_err(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_connect, (void*) on_wifi_ready));
  wifi_handle_err(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_connect, (void*) on_wifi_ready));

  wifi_init_config_t wifi_init_config = WIFI_INIT_CONFIG_DEFAULT();
  wifi_handle_err(esp_wifi_init(&wifi_init_config));
  wifi_handle_err(esp_wifi_set_storage(WIFI_STORAGE_RAM));

  wifi_config_t wifi_config = {
      .sta = {
          .ssid = CONFIG_ESP_WIFI_SSID,
          .password = CONFIG_ESP_WIFI_PASSWORD,
          .threshold.authmode = WIFI_AUTH_WPA2_PSK,
      },
  };

  wifi_handle_err(esp_wifi_set_mode(WIFI_MODE_STA));
  wifi_handle_err(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
  wifi_handle_err(esp_wifi_start());
}
