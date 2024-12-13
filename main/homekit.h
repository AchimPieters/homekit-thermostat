#include "sht40.h"

#define THERMOSTAT_DEVICE_NAME "ESP32 Thermostat"
#define THERMOSTAT_DEVICE_MANUFACTURER "Jozef Cipa"
#define THERMOSTAT_DEVICE_SERIAL "THERMOSTAT-1" // old: "NLDA4SQN1466" // TODO
#define THERMOSTAT_DEVICE_MODEL "ESP32C6" //old:  "SD466NL/A"
#define THERMOSTAT_FW_VERSION "1.0"

// Target Heating Cooling State (See chapter 9.119 in the Homekit Protocol PDF)
// 0 = OFF
// 1 = ”Heat. If the current temperature is below the target temperature then turn on heating.”
// 2 = ”Cool. If the current temperature is above the target temperature then turn on cooling.”
// 3 = ”Auto. Turn on heating or cooling to maintain temperature within the heating and cooling threshold of the target temperature.”
enum ThermostatStatus {
  THERMOSTAT_OFF = 0,
  THERMOSTAT_HEAT = 1,
  _THERMOSTAT_COOL = 2, // not supported in this project
  THERMOSTAT_AUTO = 3,
};

void homekit_init();
void homekit_update_temperature(struct TempHumidity temp_humid);