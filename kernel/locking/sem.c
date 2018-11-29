#include "sem.h"
#include "stdio.h"

int atomic_add(int *mem, int val) {
  int oldvalue;
  __asm__ volatile (
    ".set noreorder\n"
    "0:\n"
    "ll %0, %2\n"
    "addu $t0, %0, %1\n"
    "sc $t0, %2\n"
    "beqz $t0, 0b\n"
    "nop\n"
    ".set reorder\n"
    : "=&r" (oldvalue)
    : "r" (val), "m" (*mem)
    : "t0"
  );
  return oldvalue;
}

void do_semaphore_init(semaphore_t *s, int val)
{
  s->val = val;
  queue_init(&s->queue);
}

void do_semaphore_up(semaphore_t *s)
{
  if (atomic_add(&s->val, 1) < 0) {
    do_unblock_one(&s->queue);
  }
}

void do_semaphore_down(semaphore_t *s)
{
  if (atomic_add(&s->val, -1) <= 0) {
    do_block(&s->queue);
  }
}
