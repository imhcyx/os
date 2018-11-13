#include "cond.h"
#include "lock.h"

static void sp_cond_reacquire_lock(void *sparg) {
  mutex_lock_t *lock = sparg;
  do_mutex_lock_acquire(lock);
}

void do_condition_init(condition_t *condition)
{
  queue_init(&condition->queue);
}

void do_condition_wait(mutex_lock_t *lock, condition_t *condition)
{
  do_mutex_lock_release(lock);
  current_running->sparg = lock;
  current_running->spfunc = sp_cond_reacquire_lock;
  do_block(&condition->queue);
}

void do_condition_signal(condition_t *condition)
{
  do_unblock_one(&condition->queue);
}

void do_condition_broadcast(condition_t *condition)
{
  do_unblock_all(&condition->queue);
}
