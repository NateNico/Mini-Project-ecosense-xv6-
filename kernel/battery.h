#ifndef _BATTERY_H_
#define _BATTERY_H_

#define POWER_PERFORMANCE 0
#define POWER_SAVER 1

#define POWER_CLASS_INTERACTIVE 0
#define POWER_CLASS_NORMAL 1
#define POWER_CLASS_BACKGROUND 2

struct battery_status {
  int battery_level;
  int power_state;
  int threshold;
  int charging;
  int charge_ticks;
  int drain_rate;
  int estimated_ticks_left;
  int runnable_procs;
  int saver_entries;
  int performance_entries;
  int hog_cap;
};

struct battery_procinfo {
  int pid;
  int state;
  int recent_exit;
  int power_class;
  int schedule_count;
  int cpu_ticks;
  int throttle_count;
  int energy_score;
  char name[16];
};

#endif