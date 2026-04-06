#include "types.h"
#include "param.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"
#include "battery.h"

#define BATTERY_TICK_WINDOW 25
#define BATTERY_RETURN_MARGIN 15
#define BATTERY_CHARGE_STEP 3
#define BATTERY_BASE_HOG_CAP 12
#define BATTERY_RECENT_PROC_COUNT 10

struct {
  struct spinlock lock;
  int battery_level;
  int power_state;
  int threshold;
  int charging;
  int charge_ticks;
  int drain_rate;
  int estimated_ticks_left;
  int runnable_procs;
  int saver_entries;
  int performance_entries;
  int tick_window;
  int exhausted_reported;
  int recent_count;
  int recent_index;
  struct battery_procinfo recent[BATTERY_RECENT_PROC_COUNT];
} battery_state;


static int
battery_schedule_cost(struct proc *p, int saver)
{
  if(p->power_class == POWER_CLASS_INTERACTIVE)
    return 0;
  if(p->power_class == POWER_CLASS_NORMAL)
    return 1;
  return saver ? 1 : 2;
}

static int
battery_runtime_cost(struct proc *p, int saver)
{
  if(p->power_class == POWER_CLASS_INTERACTIVE)
    return 1;
  if(p->power_class == POWER_CLASS_NORMAL)
    return saver ? 1 : 2;
  return saver ? 2 : 3;
}

static void
battery_refresh_power_class(struct proc *p)
{
  if(p->cpu_ticks >= 80 || p->energy_score >= 140 || p->throttle_count >= 10)
    p->power_class = POWER_CLASS_BACKGROUND;
  else if(p->cpu_ticks >= 25 || p->energy_score >= 45 || p->schedule_count >= 12)
    p->power_class = POWER_CLASS_NORMAL;
  else
    p->power_class = POWER_CLASS_INTERACTIVE;
}


static int
clamp_percent(int value)
{
  if(value < 0)
    return 0;
  if(value > 100)
    return 100;
  return value;
}

static int
current_hog_cap_locked(void)
{
  if(battery_state.power_state == POWER_SAVER)
    return BATTERY_BASE_HOG_CAP;
  return BATTERY_BASE_HOG_CAP * 3;
}

static void
announce_mode_change(int power_state, int battery_level)
{
  if(power_state == POWER_SAVER){
    printf("[KERNEL]: Battery dropped to %d%%.\n", battery_level);
    printf("[KERNEL]: Threshold reached. Switching to POWER SAVER MODE.\n");
    printf("[KERNEL]: Scheduler nap active; background throttling increased.\n");
  } else {
    printf("[KERNEL]: Battery restored to %d%%.\n", battery_level);
    printf("[KERNEL]: Returning to PERFORMANCE mode.\n");
    printf("[KERNEL]: Background throttling relaxed.\n");
  }
}

static void
refresh_estimate_locked(void)
{
  if(battery_state.charging || battery_state.drain_rate <= 0)
    battery_state.estimated_ticks_left = -1;
  else
    battery_state.estimated_ticks_left =
      (battery_state.battery_level * BATTERY_TICK_WINDOW) / battery_state.drain_rate;
}

static int
refresh_mode_locked(void)
{
  int old_state;

  old_state = battery_state.power_state;

  if(battery_state.battery_level <= battery_state.threshold)
    battery_state.power_state = POWER_SAVER;
  else if(battery_state.battery_level >= battery_state.threshold + BATTERY_RETURN_MARGIN)
    battery_state.power_state = POWER_PERFORMANCE;

  if(old_state != battery_state.power_state){
    if(battery_state.power_state == POWER_SAVER)
      battery_state.saver_entries++;
    else
      battery_state.performance_entries++;
  }

  return battery_state.power_state != old_state;
}

static int
count_runnable_procs(void)
{
  struct proc *p;
  int count;

  count = 0;
  for(p = proc; p < &proc[NPROC]; p++){
    if(p->state == RUNNABLE || p->state == RUNNING)
      count++;
  }

  return count;
}

void
batteryinit(void)
{
  initlock(&battery_state.lock, "battery");

  battery_state.battery_level = 100;
  battery_state.power_state = POWER_PERFORMANCE;
  battery_state.threshold = 25;
  battery_state.charging = 0;
  battery_state.charge_ticks = 0;
  battery_state.drain_rate = 1;
  battery_state.estimated_ticks_left = 2500;
  battery_state.runnable_procs = 0;
  battery_state.saver_entries = 0;
  battery_state.performance_entries = 1;
  battery_state.tick_window = 0;
  battery_state.exhausted_reported = 0;
  battery_state.recent_count = 0;
  battery_state.recent_index = 0;
}

void
battery_record_exit(struct proc *p)
{
  struct battery_procinfo *slot;

  if(p->pid <= 0)
    return;
  if(p->name[0] == 0)
    return;
  if(p->cpu_ticks == 0 && p->schedule_count == 0 && p->energy_score == 0)
    return;

  acquire(&battery_state.lock);
  slot = &battery_state.recent[battery_state.recent_index];
  slot->pid = p->pid;
  slot->state = p->state;
  slot->recent_exit = 1;
  slot->power_class = p->power_class;
  slot->schedule_count = p->schedule_count;
  slot->cpu_ticks = p->cpu_ticks;
  slot->throttle_count = p->throttle_count;
  slot->energy_score = p->energy_score;
  slot->consecutive_skips = p->consecutive_skips;
  slot->total_skips = p->total_skips;
  safestrcpy(slot->name, p->name, sizeof(slot->name));

  battery_state.recent_index = (battery_state.recent_index + 1) % BATTERY_RECENT_PROC_COUNT;
  if(battery_state.recent_count < BATTERY_RECENT_PROC_COUNT)
    battery_state.recent_count++;
  release(&battery_state.lock);
}

void
battery_tick(void)
{
  int battery_level;
  int drain;
  int exhausted_now;
  int exhausted_changed;
  int mode_changed;
  int power_state;
  int runnable;

  runnable = count_runnable_procs();

  acquire(&battery_state.lock);
  battery_state.runnable_procs = runnable;
  battery_state.tick_window++;
  if(battery_state.tick_window < BATTERY_TICK_WINDOW){
    release(&battery_state.lock);
    return;
  }
  battery_state.tick_window = 0;

  drain = 1 + runnable / 2;
  if(battery_state.power_state == POWER_SAVER)
    drain = (drain + 1) / 2;
  if(drain < 1)
    drain = 1;

  battery_state.drain_rate = drain;
  if(battery_state.charging){
    battery_state.charge_ticks += BATTERY_TICK_WINDOW;
    battery_state.battery_level = clamp_percent(battery_state.battery_level + BATTERY_CHARGE_STEP);
  } else {
    battery_state.charge_ticks = 0;
    battery_state.battery_level = clamp_percent(battery_state.battery_level - drain);
  }

  mode_changed = refresh_mode_locked();
  refresh_estimate_locked();
  battery_level = battery_state.battery_level;
  power_state = battery_state.power_state;

  exhausted_changed = 0;
  exhausted_now = 0;
  if(battery_state.battery_level == 0 && !battery_state.charging){
    exhausted_now = 1;
    if(!battery_state.exhausted_reported){
      battery_state.exhausted_reported = 1;
      exhausted_changed = 1;
    }
  } else {
    battery_state.exhausted_reported = 0;
  }
  release(&battery_state.lock);

  if(mode_changed)
    announce_mode_change(power_state, battery_level);
  else if(exhausted_now && exhausted_changed){
    printf("[KERNEL]: BATTERY EXHAUSTED: emergency saver policy active.\n");
  }
}


void
battery_on_skip(struct proc *p)
{
  p->throttle_count++;
  p->consecutive_skips++;
  p->total_skips++;
  battery_refresh_power_class(p);
}

void
battery_on_schedule(struct proc *p)
{
  int saver;

  acquire(&battery_state.lock);
  saver = (battery_state.power_state == POWER_SAVER);
  release(&battery_state.lock);

  p->schedule_count++;
  p->consecutive_skips = 0;
  p->energy_score += battery_schedule_cost(p, saver);
  battery_refresh_power_class(p);
}

/*
void
battery_on_schedule(struct proc *p)
{
  int saver;

  acquire(&battery_state.lock);
  saver = (battery_state.power_state == POWER_SAVER);
  release(&battery_state.lock);

  p->schedule_count++;
  if(p->power_class == POWER_CLASS_BACKGROUND)
    p->energy_score += saver ? 1 : 2;
  else if(p->power_class == POWER_CLASS_NORMAL)
    p->energy_score += 1;
}
*/

void
battery_proc_tick(struct proc *p)
{
  int saver;

  acquire(&battery_state.lock);
  saver = (battery_state.power_state == POWER_SAVER);
  release(&battery_state.lock);

  p->cpu_ticks++;
  p->energy_score += battery_runtime_cost(p, saver);
  battery_refresh_power_class(p);
}

/*
void
battery_proc_tick(struct proc *p)
{
  int saver;

  acquire(&battery_state.lock);
  saver = (battery_state.power_state == POWER_SAVER);
  release(&battery_state.lock);

  p->cpu_ticks++;
  p->energy_score += saver ? 1 : 2;
}
*/

int
battery_is_saver(void)
{
  int saver;

  acquire(&battery_state.lock);
  saver = (battery_state.power_state == POWER_SAVER);
  release(&battery_state.lock);

  return saver;
}

int
battery_should_skip(struct proc *p)
{
  int hog_cap;
  int saver;

  acquire(&battery_state.lock);
  saver = (battery_state.power_state == POWER_SAVER);
  hog_cap = current_hog_cap_locked();
  release(&battery_state.lock);

  if(!saver)
    return 0;
  if(p->power_class == POWER_CLASS_INTERACTIVE)
    return 0;

  if(p->power_class == POWER_CLASS_BACKGROUND){
    if(p->cpu_ticks > hog_cap && ((ticks + p->pid) % 3) != 0)
      return 1;
  } else if(p->cpu_ticks > hog_cap * 2 && ((ticks + p->pid) % 2) != 0){
    return 1;
  }

  return 0;
}

/*
int
battery_should_skip(struct proc *p)
{
  int hog_cap;
  int saver;

  acquire(&battery_state.lock);
  saver = (battery_state.power_state == POWER_SAVER);
  hog_cap = current_hog_cap_locked();
  release(&battery_state.lock);

  if(!saver)
    return 0;
  if(p->power_class == POWER_CLASS_INTERACTIVE)
    return 0;

  if(p->power_class == POWER_CLASS_BACKGROUND){
    if(p->cpu_ticks > hog_cap && ((ticks + p->pid) % 3) != 0)
      return 1;
  } else if(p->cpu_ticks > hog_cap * 2 && ((ticks + p->pid) % 2) != 0){
    return 1;
  }

  return 0;
}
*/

void
battery_scheduler_pause(void)
{
  asm volatile("wfi");
}

int
battery_set_threshold(int percent)
{
  int battery_level;
  int mode_changed;
  int power_state;

  if(percent < 5 || percent > 95)
    return -1;

  acquire(&battery_state.lock);
  battery_state.threshold = percent;
  mode_changed = refresh_mode_locked();
  refresh_estimate_locked();
  battery_level = battery_state.battery_level;
  power_state = battery_state.power_state;
  release(&battery_state.lock);
  if(mode_changed)
    announce_mode_change(power_state, battery_level);
  return 0;
}

int
battery_set_level(int percent)
{
  int battery_level;
  int mode_changed;
  int power_state;

  if(percent < 0 || percent > 100)
    return -1;

  acquire(&battery_state.lock);
  battery_state.battery_level = percent;
  mode_changed = refresh_mode_locked();
  refresh_estimate_locked();
  battery_level = battery_state.battery_level;
  power_state = battery_state.power_state;
  release(&battery_state.lock);
  if(mode_changed)
    announce_mode_change(power_state, battery_level);
  return 0;
}

int
battery_set_charging(int charging)
{
  if(charging != 0 && charging != 1)
    return -1;

  acquire(&battery_state.lock);
  battery_state.charging = charging;
  if(!charging)
    battery_state.charge_ticks = 0;
  refresh_estimate_locked();
  release(&battery_state.lock);
  return 0;
}

int
battery_get_status(uint64 addr)
{
  struct proc *p;
  struct battery_status snapshot;

  p = myproc();

  acquire(&battery_state.lock);
  snapshot.battery_level = battery_state.battery_level;
  snapshot.power_state = battery_state.power_state;
  snapshot.threshold = battery_state.threshold;
  snapshot.charging = battery_state.charging;
  snapshot.charge_ticks = battery_state.charge_ticks;
  snapshot.drain_rate = battery_state.drain_rate;
  snapshot.estimated_ticks_left = battery_state.estimated_ticks_left;
  snapshot.runnable_procs = battery_state.runnable_procs;
  snapshot.saver_entries = battery_state.saver_entries;
  snapshot.performance_entries = battery_state.performance_entries;
  snapshot.hog_cap = current_hog_cap_locked();
  release(&battery_state.lock);

  if(copyout(p->pagetable, addr, (char *)&snapshot, sizeof(snapshot)) < 0)
    return -1;
  return 0;
}

int
battery_get_procs(uint64 addr, int max)
{
  struct proc *current;
  struct proc *p;
  struct battery_procinfo snapshots[NPROC];
  int count;

  if(max < 0)
    return -1;
  if(max > NPROC)
    max = NPROC;

  count = 0;
  for(p = proc; p < &proc[NPROC] && count < max; p++){
    acquire(&p->lock);
    if(p->state != UNUSED){
      snapshots[count].pid = p->pid;
      snapshots[count].state = p->state;
      snapshots[count].recent_exit = 0;
      snapshots[count].power_class = p->power_class;
      snapshots[count].schedule_count = p->schedule_count;
      snapshots[count].cpu_ticks = p->cpu_ticks;
      snapshots[count].throttle_count = p->throttle_count;
      snapshots[count].energy_score = p->energy_score;
      snapshots[count].consecutive_skips = p->consecutive_skips;
      snapshots[count].total_skips = p->total_skips;
      safestrcpy(snapshots[count].name, p->name, sizeof(snapshots[count].name));
      count++;
    }
    release(&p->lock);
  }

  acquire(&battery_state.lock);
  for(int i = 0; i < battery_state.recent_count && count < max; i++){
    int index = battery_state.recent_index - 1 - i;
    if(index < 0)
      index += BATTERY_RECENT_PROC_COUNT;
    snapshots[count] = battery_state.recent[index];
    count++;
  }
  release(&battery_state.lock);

  current = myproc();
  if(copyout(current->pagetable, addr, (char *)snapshots,
             count * sizeof(struct battery_procinfo)) < 0)
    return -1;
  return count;
}