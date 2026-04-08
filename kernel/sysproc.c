#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"
#include "vm.h"
#include "battery.h"

uint64
sys_setpowerthreshold(void)
{
  int percent;

  argint(0, &percent);

  return battery_set_threshold(percent);
}

uint64
sys_getpowerstatus(void)
{
  uint64 addr;

  argaddr(0, &addr);

  return battery_get_status(addr);
}

uint64
sys_setcharging(void)
{
  int on;

  argint(0, &on);
  return battery_set_charging(on);
}

uint64
sys_setbatterylevel(void)
{
  int percent;

  argint(0, &percent);
  return battery_set_level(percent);
}

uint64
sys_setpowerclass(void)
{
  int class_id;
  struct proc *p;

  argint(0, &class_id);
  if(class_id < POWER_CLASS_INTERACTIVE || class_id > POWER_CLASS_BACKGROUND)
    return -1;

  p = myproc();
  acquire(&p->lock);
  p->power_class = class_id;
  release(&p->lock);
  return 0;
}

uint64
sys_getbatteryprocs(void)
{
  uint64 addr;
  int max;

  argaddr(0, &addr);
  argint(1, &max);
  return battery_get_procs(addr, max);
}

uint64
sys_exit(void)
{
  int n;
  argint(0, &n);
  kexit(n);
  return 0;  // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return kfork();
}

uint64
sys_wait(void)
{
  uint64 p;
  argaddr(0, &p);
  return kwait(p);
}

uint64
sys_sbrk(void)
{
  uint64 addr;
  int t;
  int n;

  argint(0, &n);
  argint(1, &t);
  addr = myproc()->sz;

  if(t == SBRK_EAGER || n < 0) {
    if(growproc(n) < 0) {
      return -1;
    }
  } else {
    // Lazily allocate memory for this process: increase its memory
    // size but don't allocate memory. If the processes uses the
    // memory, vmfault() will allocate it.
    if(addr + n < addr)
      return -1;
    if(addr + n > TRAPFRAME)
      return -1;
    myproc()->sz += n;
  }
  return addr;
}

uint64
sys_pause(void)
{
  int n;
  uint ticks0;

  argint(0, &n);
  if(n < 0)
    n = 0;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(killed(myproc())){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

uint64
sys_kill(void)
{
  int pid;

  argint(0, &pid);
  return kkill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

uint64
sys_waitpowerchange(void)
{
  int old_state;
  argint(0, &old_state);
  return battery_wait_power_change(old_state);
}