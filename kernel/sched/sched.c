#include "lock.h"
#include "time.h"
#include "stdio.h"
#include "sched.h"
#include "queue.h"
#include "screen.h"

pcb_t pcb[NUM_MAX_TASK];
int pcbcount = 0;

queue_t ready_queue;
queue_t sleep_queue;

/* current running task PCB */
pcb_t *current_running = NULL;

/* global process id */
pid_t process_id = 1;

#define STACK_ALLOC_TOP 0xa0f00000
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
pcb_t *pcb_alloc(task_type_t type)
{
  pcb_t *proc;

  // TODO: reuse pcb of exited task

  if (pcbcount >= NUM_MAX_TASK)
    return NULL;

  proc = &pcb[pcbcount++];
  proc->pid = process_id++;
  proc->type = type;
  proc->status = TASK_INIT;

  return proc;
}

// this is the real entry point for each task
static void task_init()
{
  ((void(*)())current_running->entrypoint)();
  // the task should be killed here
}

pcb_t *new_task(struct task_info *task)
{
  pcb_t *proc;

  proc = pcb_alloc(task->type);
  if (!proc) {
    printk("failed to alloc pcb\n");
    return NULL;
  }

  // alloc kernel stack
  proc->kernel_stack_top = alloc_stack();
  // set sp
  proc->kernel_context.regs[29] = proc->kernel_stack_top;
  // set ra
  proc->kernel_context.regs[31] = (uint32_t)task_init;
  // set epc
  proc->kernel_context.cp0_epc = (uint32_t)task_init;
  proc->entrypoint = task->entry_point;
  proc->priority = task->priority;
  proc->dynamic_priority = 0;
  proc->status = TASK_READY;

  // add to ready queue
  queue_push(&ready_queue, proc);

  return proc;
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
#if 0
  // print priorities for debugging
  {
    int i;
    const char status[5] = {'I', 'B', 'R', 'Y', 'E'};
    vt100_move_cursor(1, 11);
    for (i=0; i<pcbcount; i++)
      printk("%d    ", pcb[i].priority);
    vt100_move_cursor(1, 12);
    for (i=0; i<pcbcount; i++)
      printk("%d    ", pcb[i].dynamic_priority);
    vt100_move_cursor(1, 13);
    for (i=0; i<pcbcount; i++)
      printk("%c    ", status[pcb[i].status]);
  }
#endif
  // restore screen cursor
  screen_cursor_x = current_running->cursor_x;
  screen_cursor_y = current_running->cursor_y;
}

void do_sleep(uint32_t sleep_time)
{
  current_running->status = TASK_BLOCKED;
  current_running->wakeuptime = get_timer() + sleep_time * 10000; // ?
  queue_push(&sleep_queue, current_running);
  scheduler();
}

void do_block(queue_t *queue)
{
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
