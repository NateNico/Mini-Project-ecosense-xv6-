#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/param.h"
#include "kernel/battery.h"
#include "user/user.h"

static char *
class_name(int class_id)
{
  if(class_id == POWER_CLASS_INTERACTIVE)
    return "interactive";
  if(class_id == POWER_CLASS_BACKGROUND)
    return "background";
  return "normal";
}

static char *
state_name(int state)
{
  static char *states[] = {
    [0] "unused",
    [1] "used",
    [2] "sleep",
    [3] "runble",
    [4] "run",
    [5] "zombie",
  };

  if(state >= 0 && state < 6)
    return states[state];
  return "?";
}

static void
sort_energy(struct battery_procinfo *rows, int count)
{
  int i;
  int j;

  for(i = 0; i < count; i++){
    for(j = i + 1; j < count; j++){
      if(rows[j].energy_score > rows[i].energy_score){
        struct battery_procinfo tmp = rows[i];
        rows[i] = rows[j];
        rows[j] = tmp;
      }
    }
  }
}

int
main(int argc, char *argv[])
{
  struct battery_procinfo rows[NPROC];
  int count;
  int limit;
  int watch;

  watch = argc > 1 && argv[1][0] == 'w';
  limit = 8;

  while(1){
    count = getbatteryprocs(rows, NPROC);
    if(count < 0){
      printf("batterytop: getbatteryprocs failed\n");
      exit(1);
    }

    sort_energy(rows, count);
    if(count < limit)
      limit = count;

    printf("\n=== BatteryOS Process Energy ===\n");
    for(int i = 0; i < limit; i++){
      printf("pid %d %s %s class=%s energy=%d cpu=%d sched=%d throttle=%d\n",
             rows[i].pid,
             rows[i].name,
             state_name(rows[i].state),
             class_name(rows[i].power_class),
             rows[i].energy_score,
             rows[i].cpu_ticks,
             rows[i].schedule_count,
             rows[i].throttle_count);
    }

    if(!watch)
      break;
    pause(10);
  }

  exit(0);
}