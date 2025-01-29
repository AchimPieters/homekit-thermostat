#ifndef SHT40_H
#define SHT40_H
struct TempHumidity {
  float temperature;
  float humidity;
};

void sht40_init();
struct TempHumidity sht40_measure_temp();
#endif