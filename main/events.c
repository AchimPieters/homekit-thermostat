#include <esp_err.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include "events.h"

// Main event that is used internally by the application
ESP_EVENT_DEFINE_BASE(HOMEKIT_THERMOSTAT_EVENT);

static esp_event_loop_handle_t eventloop;

void eventloop_init(eventloop_handler evt_handler) {
  ESP_LOGI("EVENTLOOP", "Initializing Homekit Thermostat event loop");

  esp_event_loop_args_t args = {
      .queue_size = 20,                                  // Maximum number of events in the queue
      .task_name = "eventloop_task",
      .task_priority = uxTaskPriorityGet(NULL),         // Same priority as current task
      .task_stack_size = configMINIMAL_STACK_SIZE * 3,  // Stack size for the event task
      .task_core_id = tskNO_AFFINITY                    // Allow running on any core
  };

  ESP_ERROR_CHECK(esp_event_loop_create(&args, &eventloop));
  ESP_ERROR_CHECK(esp_event_handler_register_with(eventloop, HOMEKIT_THERMOSTAT_EVENT, ESP_EVENT_ANY_ID, evt_handler, NULL));
}

void eventloop_dispatch(HomekitThermostatEventID event_id, const void* event_data, size_t event_data_size) {
  ESP_ERROR_CHECK(esp_event_post_to(eventloop, HOMEKIT_THERMOSTAT_EVENT, event_id, event_data, event_data_size, portMAX_DELAY));
}