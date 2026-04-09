#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/battery.h"
#include "kernel/fcntl.h"
#include "user/user.h"

static int streq(char *a, char *b){ return strcmp(a, b) == 0; }

static void
usage(void)
{
  printf("usage: powerctl status\n");
  printf("       powerctl threshold <5-95>\n");
  printf("       powerctl criticalthreshold <1-50>\n");
  printf("       powerctl level <0-100>\n");
  printf("       powerctl charge <0|1>\n");
  printf("       powerctl class <interactive|normal|background>\n");
}

int
main(int argc, char *argv[])
{
  struct battery_status status;

  if(argc < 2){ usage(); exit(1); }

  if(streq(argv[1], "status")){
    if(getpowerstatus(&status) < 0){
      printf("powerctl: getpowerstatus failed\n");
      exit(1);
    }
    printf("battery=%d mode=%d threshold=%d critical_threshold=%d "
           "charging=%d drain=%d load=%d\n",
           status.battery_level, status.power_state, status.threshold,
           status.critical_threshold, status.charging,
           status.drain_rate, status.runnable_procs);
    exit(0);
  }

  if(argc < 3){ usage(); exit(1); }

  if(streq(argv[1], "threshold")){
    int val = atoi(argv[2]);
    if(setpowerthreshold(val) < 0)
      exit(1);
    int fd = open("/battery.conf", O_CREATE | O_WRONLY);
    if(fd >= 0){
      write(fd, &val, sizeof(val));
      close(fd);
    }
    exit(0);
  }
  if(streq(argv[1], "criticalthreshold"))
    exit(setcriticalthreshold(atoi(argv[2])) < 0);
  if(streq(argv[1], "level"))
    exit(setbatterylevel(atoi(argv[2])) < 0);
  if(streq(argv[1], "charge"))
    exit(setcharging(atoi(argv[2])) < 0);
  if(streq(argv[1], "class")){
    if(streq(argv[2], "interactive"))
      exit(setpowerclass(POWER_CLASS_INTERACTIVE) < 0);
    if(streq(argv[2], "normal"))
      exit(setpowerclass(POWER_CLASS_NORMAL) < 0);
    if(streq(argv[2], "background"))
      exit(setpowerclass(POWER_CLASS_BACKGROUND) < 0);
  }

  usage();
  exit(1);
}