#include "barrier.h"

void do_barrier_init(barrier_t *barrier, int goal)
{
  barrier->num = goal;
  barrier->current = 0;
  queue_init(&barrier->queue);
}

void do_barrier_wait(barrier_t *barrier)
{
  if (atomic_add(&barrier->current, 1)+1 >= barrier->num) {
    barrier->current = 0;
    do_unblock_all(&barrier->queue);
  }
  else {
    do_block(&barrier->queue);
  }
}
