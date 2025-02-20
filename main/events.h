#include <esp_event.h>

typedef enum {
  // Trigerred when WiFi credentials are not configured
  HOMEKIT_THERMOSTAT_WIFI_REQUEST_PROVISIONING,
  // Trigerred after thermostat is connected to the WiFi
  HOMEKIT_THERMOSTAT_WIFI_CONNECTED,
  // Trigerred when thermostat loses WiFi connection
  HOMEKIT_THERMOSTAT_WIFI_DISCONNECTED,
  // Trigerred when the initialization process starts (after WiFi credentials are received)
  HOMEKIT_THERMOSTAT_INIT_STARTED,
  // Trigerred when there is an update during the initialization process (e.g. Wifi connected, Homekit started, ...)
  HOMEKIT_THERMOSTAT_LOG,
  // Trigerred when the initialization has completed, so the thermostat can start working
  HOMEKIT_THERMOSTAT_INIT_DONE,
} HomekitThermostatEventID;

typedef void (*eventloop_handler)(void*, esp_event_base_t, int32_t, void*);

void eventloop_init(eventloop_handler);
void eventloop_dispatch(HomekitThermostatEventID event_id, const void* event_data, size_t event_data_size);
