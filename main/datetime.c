#include <string.h>
#include <sys/time.h>
#include <time.h>
#include "esp_attr.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif_sntp.h"
#include "esp_sntp.h"
#include "esp_system.h"
#include "lwip/ip_addr.h"
#include "time.h"
#include "events.h"

static const char *TAG = "TIME";

void datetime_init(void) {
  ESP_LOGI(TAG, "Fetching current time over NTP.");

  // init NTP to fetch current time from the server
  esp_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG("pool.ntp.org");
  esp_netif_sntp_init(&config);

  int retry = 0;
  const int retry_count = 15;
  while (esp_netif_sntp_sync_wait(2000 / portTICK_PERIOD_MS) == ESP_ERR_TIMEOUT && ++retry < retry_count) {
    char msg[50];
    snprintf(msg, sizeof(msg), "Waiting for system time to be set... (%d/%d)", retry, retry_count);
    eventloop_dispatch(HOMEKIT_THERMOSTAT_INIT_UPDATE, msg, strlen(msg) + 1);
  }

  esp_netif_sntp_deinit();

  // set timezone
  setenv("TZ", "CET-1CEST,M3.5.0/2,M10.5.0/3", 1);  // Prague with the daylight saving time config
  tzset();
}

struct tm datetime_now() {
  time_t now = time(NULL);
  struct tm timeinfo;
  localtime_r(&now, &timeinfo);

  return timeinfo;
}

void datetime_datef(char* buff, size_t buff_size, struct tm* timeinfo) {
  strftime(buff, buff_size, "%d.%m.%Y\n%A", timeinfo);
}

void datetime_timef(char* buff, size_t buff_size, struct tm* timeinfo) {
  strftime(buff, buff_size, "%H:%M", timeinfo);
}