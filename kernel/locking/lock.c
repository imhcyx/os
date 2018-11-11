#include "lock.h"
#include "sched.h"
#include "syscall.h"

uint32_t atomic_xchg(uint32_t *mem, uint32_t newvalue) {
  uint32_t oldvalue;
  __asm__ volatile (
    ".set noreorder\n"
    "retry:\n"
    "ll %0, %2\n"
    "sc %1, %2\n"
    "beqz %1, retry\n"
    "nop\n"
    ".set reorder\n"
    : "=&r" (oldvalue)
    : "r" (newvalue), "m" (*mem)
  );
  return oldvalue;
}

void spin_lock_init(spin_lock_t *lock)
{
    lock->status = UNLOCKED;
}

void spin_lock_acquire(spin_lock_t *lock)
{
    while (LOCKED == lock->status)
    {
    };
    lock->status = LOCKED;
}

void spin_lock_release(spin_lock_t *lock)
{
    lock->status = UNLOCKED;
}

void do_mutex_lock_init(mutex_lock_t *lock)
{
  lock->status = UNLOCKED;
  lock->owner = NULL;
  queue_init(&lock->queue);
  // register mutex lock
  extern mutex_lock_t **mutex_list;
  int i = 0;
  while (mutex_list[i]) i++;
  mutex_list[i] = lock;
}

void do_mutex_lock_acquire(mutex_lock_t *lock)
{
  if (atomic_xchg((uint32_t*)&lock->status, LOCKED) == UNLOCKED) {
    lock->owner = current_running;
    return;
  }
  // lock->owner must be cleared if the lock is previously unlocked
  // otherwise race conditions may happen here
  // e.g. T1:release -> (unlocked) -> T2:acquire -> T1:acquire
  if (lock->owner == current_running)
    return;
  do_block(&lock->queue);
}

void do_mutex_lock_release(mutex_lock_t *lock)
{
  if (lock->status != LOCKED || lock->owner != current_running)
    return;
  if (queue_is_empty(&lock->queue)) {
    lock->status = UNLOCKED;
    lock->owner = NULL;
  }
  else
    lock->owner = do_unblock_one(&lock->queue);
}
