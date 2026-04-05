#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/battery.h"
#include "user/user.h"

static char *
status_name(int charging)
{
  if(charging)
    return "CHARGING";
  return "UNPLUGGED";
}

static char *
mode_name(int mode)
{
  if(mode == POWER_CRITICAL)
    return "!! CRITICAL !!";
  if(mode == POWER_SAVER)
    return "POWER SAVER";
  return "PERFORMANCE";
}

static int
charging_too_long(struct battery_status *status)
{
  if(!status->charging)
    return 0;
  if(status->battery_level < 90)
    return 0;
  return status->charge_ticks >= 150;
}

static void
print_bar(int percent)
{
  int filled;

  filled = (percent + 9) / 10;
  if(filled < 0)
    filled = 0;
  if(filled > 10)
    filled = 10;

  printf("[");
  for(int i = 0; i < filled; i++)
    printf("#");
  for(int i = filled; i < 10; i++)
    printf("_");
  printf("]");
}

int
main(void)
{
  struct battery_status status;

  if(getpowerstatus(&status) < 0){
    printf("battery: getpowerstatus failed\n");
    exit(1);
  }

  printf("--- BatteryOS Energy Monitor ---\n");
  printf("Current Charge: %d%% ", status.battery_level);
  print_bar(status.battery_level);
  printf("\n");
  printf("Status: %s\n", status_name(status.charging));
  printf("Current Mode: %s\n", mode_name(status.power_state));
  printf("Threshold: %d%% ", status.threshold);
  if(status.battery_level <= status.threshold)
    printf("(ACTIVE: System is in SAVER mode)\n");
  else
    printf("(OK: System can remain in PERFORMANCE mode)\n");
  printf("Crit Threshold: %d%% ", status.critical_threshold);
  if(status.power_state == POWER_CRITICAL)
    printf("(ACTIVE: only interactive processes running)\n");
  else
    printf("(OK)\n");
  if(status.estimated_ticks_left < 0)
    printf("Estimated Time Remaining: charging\n");
  else
    printf("Estimated Time Remaining: %d ticks\n", status.estimated_ticks_left);

  if(charging_too_long(&status))
    printf("BatteryOS: unplug it already. You have been charging longer than needed.\n");

  printf("\n");

  exit(0);
}