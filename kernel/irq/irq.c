#include "irq.h"
#include "time.h"
#include "sched.h"
#include "string.h"

void panic(char *desc) {
  // PANIC
  uint32_t status, cause, epc, badvaddr;
  int i;
  status = current_running->context.cp0_status;
  cause = current_running->context.cp0_cause;
  epc = current_running->context.cp0_epc;
  badvaddr = current_running->context.cp0_badvaddr;
  printk("\n");
  printk(" *** PANIC ***\n");
  printk(" %s\n", desc);
  printk(" pid=%d\n", current_running->pid);
  printk(" status=0x%08x cause=0x%08x epc=0x%08x badvaddr=0x%08x\n", status, cause, epc, badvaddr);
  printk(" registers:\n");
  for (i=0; i<32; i+=4)
    printk(" %02d: 0x%08x %02d: 0x%08x %02d: 0x%08x %02d: 0x%08x\n",
        i, current_running->context.regs[i],
        i+1, current_running->context.regs[i+1],
        i+2, current_running->context.regs[i+2],
        i+3, current_running->context.regs[i+3]);
  for(;;);
}

static void irq_timer()
{
  // call scheduler
  scheduler();
  // reflush screen
  screen_reflush();
  // increase time count
  time_elapsed += CLK_INT_CYCLE;
  // reset timer
  __asm__ volatile (
    "mtc0 %0, $11\n"
    "mtc0 $zero, $9\n"
    :: "r" (CLK_INT_CYCLE)
  );
}

static void (*irq_handler[8])() = {
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  irq_timer
};

void interrupt_helper()
{
  int i;
  uint32_t cause = current_running->context.cp0_cause;
  for (i=0; i<8; i++)
    if ((cause>>(i+8))&0x1)
      if (irq_handler[i])
        irq_handler[i]();
}

void before_eret() {
  pcb_t *proc;
retry:
  proc = current_running;
  if (current_running->spfunc) {
    current_running->spfunc(current_running->sparg);
    // maybe current process has changed (e.g. after exit())
    if (current_running != proc) goto retry;
    current_running->spfunc = NULL;
  }
}

void other_exception_handler()
{
  panic("unhandled exception");
}
