#ifndef _BATTERY_H_
#define _BATTERY_H_

#define POWER_PERFORMANCE 0
#define POWER_SAVER       1
#define POWER_CRITICAL    2   // battery critically low, only interactive runs

#define POWER_CLASS_INTERACTIVE 0
#define POWER_CLASS_NORMAL      1
#define POWER_CLASS_BACKGROUND  2

struct battery_status {
  int battery_level;
  int power_state;
  int threshold;
  int critical_threshold;     // new: below this -> POWER_CRITICAL
  int charging;
  int charge_ticks;
  int drain_rate;
  int estimated_ticks_left;
  int runnable_procs;
  int saver_entries;
  int performance_entries;
  int critical_entries;       // new: how many times critical mode was entered
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
  int consecutive_skips;
  int total_skips;
  char name[16];
};

#endif
