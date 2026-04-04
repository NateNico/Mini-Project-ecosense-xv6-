#include "types.h"
#include "param.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"
#include "ecosense.h"

struct {
  struct spinlock lock;
  struct ecosense_data data;
  int  eco_mode_active;
  uint eco_mode_until;
} ecosense_state;

static void
ecosense_update_alerts(void)
{
  int old_temp   = ecosense_state.data.temp_alert;
  int old_air    = ecosense_state.data.air_alert;
  int old_energy = ecosense_state.data.energy_alert;

  ecosense_state.data.temp_alert   = (ecosense_state.data.temperature  > 35);
  ecosense_state.data.air_alert    = (ecosense_state.data.air_quality   < 25);
  ecosense_state.data.energy_alert = (ecosense_state.data.energy_usage  > 85);

  // Only re-arm eco mode on a fresh trigger, not while it's already firing
  int newly_triggered =
    (!old_temp   && ecosense_state.data.temp_alert)   ||
    (!old_air    && ecosense_state.data.air_alert)    ||
    (!old_energy && ecosense_state.data.energy_alert);

  if(newly_triggered){
    ecosense_state.eco_mode_active = 1;
    ecosense_state.eco_mode_until  = ticks + ECO_MODE_TICKS;
  }
}

void
ecosenseinit(void)
{
  initlock(&ecosense_state.lock, "ecosense");

  ecosense_state.data.temperature  = 20;
  ecosense_state.data.air_quality  = 50;
  ecosense_state.data.energy_usage = 70;
  ecosense_state.data.temp_alert   = 0;
  ecosense_state.data.air_alert    = 0;
  ecosense_state.data.energy_alert = 0;
  ecosense_state.data.eco_mode     = 0;
  ecosense_state.data.eco_ticks    = 0;
  ecosense_state.data.norm_ticks   = 0;
  ecosense_state.eco_mode_active   = 0;
  ecosense_state.eco_mode_until    = 0;
}

int
ecosense_set(int type, int value)
{
  acquire(&ecosense_state.lock);

  switch(type){
  case SENSOR_TEMP:
    ecosense_state.data.temperature = value;
    break;
  case SENSOR_AIR:
    ecosense_state.data.air_quality = value;
    break;
  case SENSOR_ENERGY:
    ecosense_state.data.energy_usage = value;
    break;
  case SENSOR_ECO_TICK:
    ecosense_state.data.eco_ticks++;
    break;
  case SENSOR_NORM_TICK:
    ecosense_state.data.norm_ticks++;
    break;
  default:
    release(&ecosense_state.lock);
    return -1;
  }

  ecosense_update_alerts();
  release(&ecosense_state.lock);
  return 0;
}

int
ecosense_get(uint64 addr)
{
  struct proc *p = myproc();
  struct ecosense_data snapshot;

  acquire(&ecosense_state.lock);

  if(ecosense_state.eco_mode_active && ticks >= ecosense_state.eco_mode_until)
    ecosense_state.eco_mode_active = 0;

  ecosense_state.data.eco_mode = ecosense_state.eco_mode_active;
  snapshot = ecosense_state.data;

  release(&ecosense_state.lock);

  if(copyout(p->pagetable, addr, (char *)&snapshot, sizeof(snapshot)) < 0)
    return -1;

  return 0;
}

int
ecosense_eco_mode_active(void)
{
  int active;

  acquire(&ecosense_state.lock);
  if(ecosense_state.eco_mode_active && ticks >= ecosense_state.eco_mode_until)
    ecosense_state.eco_mode_active = 0;
  active = ecosense_state.eco_mode_active;
  release(&ecosense_state.lock);

  return active;
}