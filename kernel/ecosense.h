#ifndef _ECOSENSE_H_
#define _ECOSENSE_H_

#define SENSOR_TEMP      1
#define SENSOR_AIR       2
#define SENSOR_ENERGY    3
#define SENSOR_ECO_TICK  4   // eco worker reports a completed loop
#define SENSOR_NORM_TICK 5   // norm worker reports a completed loop

// Eco mode stays active for ~60 seconds (xv6 timer runs at ~100 Hz)
#define ECO_MODE_TICKS 200

struct ecosense_data {
  int temperature;
  int air_quality;
  int energy_usage;
  int temp_alert;
  int air_alert;
  int energy_alert;
  int eco_mode;       // 1 if eco mode is currently active
  int eco_ticks;      // total loops completed by eco worker
  int norm_ticks;     // total loops completed by norm worker
};

#endif