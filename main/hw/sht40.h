#ifndef SHT40_H
#define SHT40_H
typedef struct {
  float temperature;
  float humidity;
} TempHumidity;

void sht40_init();
TempHumidity sht40_measure_temp();
#endif