#include "lock.h"
#include "time.h"
#include "stdio.h"
#include "sched.h"
#include "queue.h"
#include "screen.h"

pcb_t pcb[NUM_MAX_TASK];

queue_t ready_queue;
queue_t sleep_queue;

/* current running task PCB */
pcb_t *current_running = NULL;

/* global process id */
pid_t process_id = 0;

#define STACK_ALLOC_TOP 0xa0fff000
#define STACK_ALLOC_SIZE 0x1000
uint32_t stack_alloc_ptr = STACK_ALLOC_TOP;

// alloc a stack
static uint32_t alloc_stack()
{
  uint32_t stack = stack_alloc_ptr;
  stack_alloc_ptr -= STACK_ALLOC_SIZE;
  return stack;
}

// alloc a pcb
static pcb_t *pcb_alloc(task_type_t type)
{
  int i;
  pcb_t *proc = NULL;

  for (i=0; i<NUM_MAX_TASK; i++) {
    if (pcb[i].status == TASK_UNUSED) {
      proc = &pcb[i];
      break;
    }
  }
  if (!proc) return NULL;

  proc->pid = process_id++;
  proc->type = type;
  proc->status = TASK_INIT;
  proc->spfunc = NULL;
  proc->queue = NULL;

  proc->prev = NULL;
  proc->next = NULL;

  queue_init(&proc->waitqueue);

  memset(&proc->pgd, 0, sizeof(proc->pgd));

  return proc;
}

static void pcb_free(pcb_t * pcb) {
  pcb->status = TASK_UNUSED;
}

// this is the real entry point for each task
static void task_init()
{
  ((void(*)())current_running->entrypoint)();
  // the task should be killed here
  for(;;) sys_exit();
}

void sched_init()
{
  pcb_t *proc;
  int i;

  // init queues
  queue_init(&ready_queue);
  queue_init(&sleep_queue);

  // create initial kernel process
  proc = pcb_alloc(KERNEL_PROCESS);
  if (!proc) {
    printk("failed to alloc pcb\n");
    for(;;);
  }
  strcpy(&proc->name, "kernel");
  proc->priority = 0;
  proc->dynamic_priority = 0;
  proc->status = TASK_RUNNING;
  current_running = proc;
}

void scheduler(void)
{
  pcb_t *proc, *x;
  int maxpriority = -1;

  // save screen cursor
  current_running->cursor_x = screen_cursor_x;
  current_running->cursor_y = screen_cursor_y;
  // push current task into ready queue
  if (current_running->status == TASK_RUNNING)  {
    current_running->status = TASK_READY;
    queue_push(&ready_queue, current_running);
  }
  // wake up sleeping tasks
  queue_iterate(&sleep_queue, x) {
    uint32_t time = get_timer();
    if (x->wakeuptime <= time) {
      queue_remove(&sleep_queue, x);
      x->wakeuptime = 0;
      queue_push(&ready_queue, x);
    }
  }
  // panic if no ready tasks (at least we should have the idle(0) task)
  if (queue_is_empty(&ready_queue)) {
    panic("no schedulable task");
  }
  // find the first task with the highest priority
  queue_iterate (&ready_queue, x) {
    if (x->priority+x->dynamic_priority > maxpriority) {
      maxpriority = x->priority+x->dynamic_priority;
      proc = x;
    }
    x->dynamic_priority++;
  }
  queue_remove(&ready_queue, proc);
  proc->dynamic_priority = 0;
  current_running = proc;
  current_running->status = TASK_RUNNING;
  __asm__ volatile ("mtc0 %0, $10\n" :: "r" (current_running->pid));
#if 0
  {
    uint32_t entryhi;
    __asm__ volatile ("mfc0 %0, $10\n" : "=r" (entryhi));
    vt100_move_cursor(1,30);
    printk("entryhi: %08x", entryhi);
  }
  // print priorities for debugging
  {
    int i;
    const char status[] = {' ', 'I', 'B', 'R', 'Y', 'E'};
    vt100_move_cursor(1, 11);
    for (i=0; i<NUM_MAX_TASK; i++)
      if (pcb[i].status != TASK_UNUSED)
        printk("%d    ", pcb[i].priority);
    vt100_move_cursor(1, 12);
    for (i=0; i<NUM_MAX_TASK; i++)
      if (pcb[i].status != TASK_UNUSED)
        printk("%d    ", pcb[i].dynamic_priority);
    vt100_move_cursor(1, 13);
    for (i=0; i<NUM_MAX_TASK; i++)
      if (pcb[i].status != TASK_UNUSED)
        printk("%c    ", status[pcb[i].status]);
  }
#endif
  // restore screen cursor
  screen_cursor_x = current_running->cursor_x;
  screen_cursor_y = current_running->cursor_y;
}

static void sp_exit(void *sparg) {
  exit();
}

static void sp_postwait(void *sparg) {
  pcb_t *proc = sparg;
  pcb_free(proc);
}

pcb_t *spawn(struct task_info *task)
{
  pcb_t *proc;

  proc = pcb_alloc(task->type);
  if (!proc) {
    printk("failed to alloc pcb\n");
    return NULL;
  }

  // TODO: strncpy
  strcpy(&proc->name, task->name);
  // alloc stack (TODO:)
  if (!proc->kernel_stack_top)
    proc->kernel_stack_top = alloc_stack();
  if (task->type == USER_PROCESS || task->type == USER_THREAD)
    proc->stack_top = 0x7f000000;
  else
    proc->stack_top = proc->kernel_stack_top;
  // set sp
  proc->context.regs[29] = proc->stack_top;
  // set ra
  proc->context.regs[31] = (uint32_t)task_init;
  // set epc
  proc->context.cp0_epc = (uint32_t)task_init;
  proc->entrypoint = task->entry_point;
  proc->priority = task->priority;
  proc->dynamic_priority = 0;
  proc->status = TASK_READY;

  // add to ready queue
  queue_push(&ready_queue, proc);

  return proc;
}

void kill(pcb_t *proc) {
  if (proc->status != TASK_UNUSED && proc->status != TASK_EXITED) {
    proc->spfunc = sp_exit;
    if (proc->status != TASK_RUNNING && proc->status != TASK_READY) {
      queue_remove(proc->queue, proc);
      queue_push(&ready_queue, proc);
    }
  }
}

void exit() {
  int i;
  // NOTE: must be called in interrupt context
  current_running->status = TASK_EXITED;
  // release mutex locks
  for (i=0; i<16; i++)
    if (mutex_list[i] && mutex_list[i]->owner == current_running)
      do_mutex_lock_release(mutex_list[i]);
  // TODO: release other resources
  // wake up waiting processes
  do_unblock_all(&current_running->waitqueue);
  scheduler();
}

void wait(pcb_t *proc) {
  // NOTE: must be called in interrupt context
  if (proc->status != TASK_UNUSED) {
    if (proc->status == TASK_EXITED) {
      sp_postwait(proc);
    }
    else {
      current_running->sparg = proc;
      current_running->spfunc = sp_postwait;
      do_block(&proc->waitqueue);
    }
  }
}

void do_sleep(uint32_t sleep_time)
{
  // NOTE: must be called in interrupt context
  current_running->status = TASK_BLOCKED;
  current_running->wakeuptime = (get_timer() + sleep_time * 10000) % MAX_TIME; // ?
  queue_push(&sleep_queue, current_running);
  scheduler();
}

void do_block(queue_t *queue)
{
  // NOTE: must be called in interrupt context
  // block the current_running task into the queue
  current_running->status = TASK_BLOCKED;
  queue_push(queue, current_running);
  scheduler();
}

pcb_t *do_unblock_one(queue_t *queue)
{
  // unblock the task with the highest priority from the queue
  pcb_t *proc = NULL, *x;
  int p = -1;
  queue_iterate(queue, x) {
    if (x->priority > p) {
      p = x->priority;
      proc = x;
    }
  }
  if (proc) {
    queue_remove(queue, proc);
    proc->status = TASK_READY;
    queue_push(&ready_queue, proc);
  }
  return proc;
}

void do_unblock_all(queue_t *queue)
{
  // unblock all task in the queue
  pcb_t *proc;
  while (!queue_is_empty(queue)) {
    proc = queue_dequeue(queue);
    proc->status = TASK_READY;
    queue_push(&ready_queue, proc);
  }
}
