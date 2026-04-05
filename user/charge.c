#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/battery.h"
#include "user/user.h"

static void
usage(void)
{
  printf("usage: charge [0-100]\n\n");
}

int
main(int argc, char *argv[])
{
  int level;
  struct battery_status status;

  if(argc > 2){
    usage();
    exit(1);
  }

  if(argc == 1){
    if(setcharging(1) < 0){
      printf("charge: failed to enable charging\n");
      exit(1);
    }
    if(getpowerstatus(&status) < 0){
      printf("charge: failed to read current battery level\n");
      exit(1);
    }
    printf("BatteryOS charging started at %d%%.\n\n", status.battery_level);
    exit(0);
  }

  level = atoi(argv[1]);
  if(level < 0 || level > 100){
    usage();
    exit(1);
  }

  if(setbatterylevel(level) < 0 || setcharging(0) < 0){
    printf("charge: failed to set charge level\n");
    exit(1);
  }

  printf("BatteryOS battery set to %d%% and unplugged.\n\n", level);
  exit(0);
}