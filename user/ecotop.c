#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/param.h"
#include "kernel/battery.h"
#include "user/user.h"

static char *
state_name(int state)
{
  static char *states[] = {
    [0] "UNUSED",
    [1] "USED",
    [2] "SLEEPING",
    [3] "RUNNABLE",
    [4] "RUNNING",
    [5] "ZOMBIE",
  };

  if(state >= 0 && state < 6)
    return states[state];
  return "?";
}

static char *
display_state(struct battery_procinfo *row)
{
  if(row->recent_exit)
    return "DONE";
  return state_name(row->state);
}

static void
sort_energy(struct battery_procinfo *rows, int count)
{
  for(int i = 0; i < count; i++){
    for(int j = i + 1; j < count; j++){
      if(rows[j].energy_score > rows[i].energy_score){
        struct battery_procinfo tmp = rows[i];
        rows[i] = rows[j];
        rows[j] = tmp;
      }
    }
  }
}

static int
efficiency_score(struct battery_procinfo *row)
{
  int score;

  score = 100;
  score -= row->energy_score / 3;
  score -= row->throttle_count / 8;
  if(row->power_class == POWER_CLASS_BACKGROUND)
    score -= 5;
  if(score < 0)
    score = 0;
  return score;
}

static void
print_spaces(int count)
{
  for(int i = 0; i < count; i++)
    printf(" ");
}

static void
print_col(char *text, int width)
{
  int len;

  len = strlen(text);
  if(len > width)
    len = width;

  for(int i = 0; i < len; i++)
    printf("%c", text[i]);
  print_spaces(width - len);
}

static void
print_num_col(int value, int width)
{
  char buf[16];
  int len;
  int x;

  len = 0;
  x = value;
  if(x == 0)
    buf[len++] = '0';
  while(x > 0 && len < sizeof(buf) - 1){
    buf[len++] = '0' + (x % 10);
    x /= 10;
  }
  for(int i = 0; i < len / 2; i++){
    char tmp = buf[i];
    buf[i] = buf[len - 1 - i];
    buf[len - 1 - i] = tmp;
  }
  buf[len] = '\0';

  print_col(buf, width);
}

int
main(void)
{
  struct battery_procinfo rows[NPROC];
  struct battery_status status;
  int count;

  count = getbatteryprocs(rows, NPROC);
  if(count < 0 || getpowerstatus(&status) < 0){
    printf("ecotop: failed to gather BatteryOS data\n");
    exit(1);
  }

  sort_energy(rows, count);

  printf("PID   NAME             STATE      CYCLES   EFFICIENCY\n");
  printf("------------------------------------------------------\n");
  for(int i = 0; i < count && i < 8; i++){
    int cycles_used = rows[i].cpu_ticks * 1000;
    int score = efficiency_score(&rows[i]);
    char *label = rows[i].throttle_count > 0 || score < 40 ? "ENERGY HOG" : "Clean";
    if(rows[i].power_class == POWER_CLASS_INTERACTIVE && score > 80)
      label = "Efficient";

    print_num_col(rows[i].pid, 5);
    print_col(rows[i].name, 17);
    print_col(display_state(&rows[i]), 11);
    print_num_col(cycles_used, 9);
    printf("%d%% %s\n", score, label);
  }

  if(status.power_state == POWER_SAVER)
    printf("\n** SAVER MODE ACTIVE: background hogs are being throttled **\n");

  printf("\n");

  exit(0);
}