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

int
main(int argc, char *argv[])
{
  int class_id;
  int rounds;
  int burst_pause;

  class_id = POWER_CLASS_NORMAL;
  rounds = 12000000;
  burst_pause = 1;

  if(argc > 1){
    if(strcmp(argv[1], "interactive") == 0){
      class_id = POWER_CLASS_INTERACTIVE;
      rounds = 3000000;
      burst_pause = 2;
    } else if(strcmp(argv[1], "background") == 0){
      class_id = POWER_CLASS_BACKGROUND;
      rounds = 20000000;
      burst_pause = 0;
    }
  }

  if(setpowerclass(class_id) < 0){
    printf("battery_worker: setpowerclass failed\n");
    exit(1);
  }

  while(1){
    busy_loop(rounds);
    if(burst_pause > 0)
      pause(burst_pause);
  }

  exit(0);
}