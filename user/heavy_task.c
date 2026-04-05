#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/battery.h"
#include "user/user.h"

static void
busy_loop(int rounds)
{
  volatile int sink = 0;

  for(int i = 0; i < rounds; i++)
    sink += i;
}

static void
run_chunks(int chunks, int rounds)
{
  for(int i = 0; i < chunks; i++)
    busy_loop(rounds);
}

int
main(void)
{
  struct battery_status before;
  struct battery_status after;
  int start_ticks;
  int used;
  int elapsed;

  if(setpowerclass(POWER_CLASS_BACKGROUND) < 0){
    printf("heavy_task: setpowerclass failed\n");
    exit(1);
  }
  if(getpowerstatus(&before) < 0){
    printf("heavy_task: getpowerstatus failed\n");
    exit(1);
  }

  start_ticks = uptime();
  run_chunks(180, 12000000);
  elapsed = uptime() - start_ticks;

  if(getpowerstatus(&after) < 0){
    printf("heavy_task: getpowerstatus failed\n");
    exit(1);
  }

  used = before.battery_level - after.battery_level;
  if(used < 0)
    used = 0;

  printf("Task complete in %d ticks.\n", elapsed);
  printf("Battery used: %d%%\n", used);
  if(before.power_state == POWER_SAVER)
    printf("(Note: Task took longer, but BatteryOS reduced drain under saver mode.)\n");
  printf("\n");

  exit(0);
}