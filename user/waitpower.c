#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/battery.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  struct battery_status status;

  if(getpowerstatus(&status) < 0){
    printf("waitpower: getpowerstatus failed\n");
    exit(1);
  }

  int cur_state = status.power_state;
  printf("waitpower: starting, current power_state is %d\n", cur_state);

  while(1){
    int new_state = waitpowerchange(cur_state);
    if(new_state < 0){
      printf("waitpower: error or process killed\n");
      exit(1);
    }
    if(new_state != cur_state){
      if(new_state == POWER_SAVER){
        printf("\n\n>>> waitpower: NOTIFIED! Power state changed to SAVER (1) <<<\n\n");
      } else {
        printf("\n\n>>> waitpower: NOTIFIED! Power state changed to PERFORMANCE (0) <<<\n\n");
      }
      cur_state = new_state;
    }
  }

  exit(0);
}
