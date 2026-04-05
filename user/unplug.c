#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(void)
{
  if(setcharging(0) < 0){
    printf("unplug: failed to disable charging\n");
    exit(1);
  }

  printf("Charging disabled.\n\n");
  exit(0);
}
