#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/param.h"
#include "kernel/battery.h"
#include "user/user.h"

static int
spawn(char *prog, char *argv[])
{
  int pid;

  pid = fork();
  if(pid == 0){
    exec(prog, argv);
    printf("battery_demo: failed to exec %s\n", prog);
    exit(1);
  }
  return pid;
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

static char *
mode_name(int mode)
{
  if(mode == POWER_SAVER)
    return "POWER_SAVER";
  return "PERFORMANCE";
}

static char *
class_name(int class_id)
{
  if(class_id == POWER_CLASS_INTERACTIVE)
    return "interactive";
  if(class_id == POWER_CLASS_BACKGROUND)
    return "background";
  return "normal";
}

static void
print_snapshot(char *title)
{
  struct battery_status status;
  struct battery_procinfo rows[NPROC];
  int count;

  if(getpowerstatus(&status) < 0){
    printf("battery_demo: getpowerstatus failed\n");
    return;
  }
  count = getbatteryprocs(rows, NPROC);
  if(count < 0){
    printf("battery_demo: getbatteryprocs failed\n");
    return;
  }
  sort_energy(rows, count);

  printf("\n=== %s ===\n", title);
  printf("battery=%d%% mode=%s charging=%d drain=%d load=%d eta=%d\n",
         status.battery_level, mode_name(status.power_state), status.charging,
         status.drain_rate, status.runnable_procs, status.estimated_ticks_left);
  for(int i = 0; i < count && i < 4; i++)
    printf("pid %d %s class=%s energy=%d cpu=%d throttle=%d\n",
           rows[i].pid, rows[i].name, class_name(rows[i].power_class),
           rows[i].energy_score, rows[i].cpu_ticks, rows[i].throttle_count);
}

int
main(void)
{
  int pids[4];
  int n;

  n = 0;
  setpowerthreshold(35);
  setbatterylevel(32);
  setcharging(0);

  pids[n++] = spawn("battery_worker", (char*[]){"battery_worker", "interactive", 0});
  pids[n++] = spawn("battery_worker", (char*[]){"battery_worker", "normal", 0});
  pids[n++] = spawn("battery_worker", (char*[]){"battery_worker", "background", 0});
  pids[n++] = spawn("battery_worker", (char*[]){"battery_worker", "background", 0});

  printf("BatteryOS scripted demo started.\n");
  printf("Phase 1 begins below the threshold, so saver mode should throttle background workers.\n");

  pause(20);
  print_snapshot("Phase 1: Low Battery");
  pause(20);
  print_snapshot("Phase 2: Saver Mode Under Load");

  printf("\nPlugging in charger and restoring battery reserve...\n");
  setcharging(1);
  setbatterylevel(70);

  pause(20);
  print_snapshot("Phase 3: Charging Recovery");
  pause(20);
  print_snapshot("Phase 4: Back To Performance");

  printf("\nTry these manual commands next:\n");
  printf("  battery\n");
  printf("  ecotop\n");
  printf("  charge 100\n");
  printf("  heavy_task\n");

  for(int i = 0; i < n; i++)
    kill(pids[i]);
  for(int i = 0; i < n; i++)
    wait(0);

  printf("BatteryOS demo complete. Run battery_status or batterytop for manual inspection.\n");
  exit(0);
}