#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/battery.h"
#include "user/user.h"

static char *
mode_name(int mode)
{
  if(mode == POWER_SAVER)
    return "POWER_SAVER";
  return "PERFORMANCE";
}

int
main(int argc, char *argv[])
{
  struct battery_status status;
  int watch;

  watch = argc > 1 && argv[1][0] == 'w';

  while(1){
    if(getpowerstatus(&status) < 0){
      printf("battery_status: getpowerstatus failed\n");
      exit(1);
    }

    printf("\n=== BatteryOS Status ===\n");
    printf("Battery      : %d%%\n", status.battery_level);
    printf("Mode         : %s\n", mode_name(status.power_state));
    printf("Threshold    : %d%%\n", status.threshold);
    printf("Charging     : %s\n", status.charging ? "yes" : "no");
    printf("Drain rate   : %d units/window\n", status.drain_rate);
    printf("Runnable load: %d\n", status.runnable_procs);
    printf("Hog cap      : %d cpu ticks\n", status.hog_cap);
    printf("Mode flips   : saver %d, performance %d\n",
           status.saver_entries, status.performance_entries);
    if(status.estimated_ticks_left < 0)
      printf("Time to death: charging\n");
    else
      printf("Time to death: about %d timer ticks\n", status.estimated_ticks_left);

    if(!watch)
      break;
    pause(10);
  }

  exit(0);
}