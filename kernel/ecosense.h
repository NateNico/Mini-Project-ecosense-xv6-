#ifndef _ECOSENSE_H_
#define _ECOSENSE_H_

#define SENSOR_TEMP   1
#define SENSOR_AIR    2
#define SENSOR_ENERGY 3

struct ecosense_data {
  int temperature;
  int air_quality;
  int energy_usage;
  int temp_alert;
  int air_alert;
  int energy_alert;
};

#endif





