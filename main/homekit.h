#include "hw/sht40.h"

#ifndef HOMEKIT_H
#define HOMEKIT_H

#define THERMOSTAT_DEVICE_NAME "ESP32 Thermostat"
#define THERMOSTAT_DEVICE_MANUFACTURER "Jozef Cipa"
#define THERMOSTAT_DEVICE_SERIAL "THERMOSTAT-1"
#define THERMOSTAT_DEVICE_MODEL "ESP32C6"
#define THERMOSTAT_FW_VERSION "1.0"
// TODO: define these in KConfig
#define MIN_TEMP 10
#define MAX_TEMP 38 // Homekit allows up to 38 degrees

// Target Heating Cooling State (See chapter 9.119 in the Homekit Protocol PDF)
// 0 = OFF
// 1 = ”Heat. If the current temperature is below the target temperature then turn on heating.”
// 2 = ”Cool. If the current temperature is above the target temperature then turn on cooling.”
// 3 = ”Auto. Turn on heating or cooling to maintain temperature within the heating and cooling threshold of the target temperature.”
typedef enum {
  THERMOSTAT_OFF = 0,
  THERMOSTAT_HEAT = 1,
  _THERMOSTAT_COOL = 2, // not supported in this project
  _THERMOSTAT_AUTO = 3, // not supported in this project
} ThermostatStatus;

typedef struct {
  float current_temp;
  float target_temp;
  ThermostatStatus current_state;
  ThermostatStatus target_state;
} HomekitState;

void homekit_init(void (*on_homekit_update)(HomekitState state));

void homekit_set_curr_temp(TempHumidity temp_humid);
void homekit_set_target_temp(float temp);
void homekit_set_thermostat_status(ThermostatStatus status);

HomekitState homekit_get_state();

#endif