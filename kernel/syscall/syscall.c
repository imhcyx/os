#include "lock.h"
#include "sched.h"
#include "common.h"
#include "screen.h"
#include "syscall.h"

static int _spawn(int arg1, int arg2, int arg3) {
  spawn((struct task_info*)arg1);
  return 0;
}

static int _kill(int arg1, int arg2, int arg3) {
  int i;
  for (i=0; i<NUM_MAX_TASK; i++) {
    if (pcb[i].status != TASK_UNUSED && pcb[i].pid == arg1) {
      kill(&pcb[i]);
      break;
    }
  }
  return 0;
}

static int _exit(int arg1, int arg2, int arg3) {
  exit();
  return 0;
}

static int _waitpid(int arg1, int arg2, int arg3) {
  int i;
  for (i=0; i<NUM_MAX_TASK; i++) {
    if (pcb[i].status != TASK_UNUSED && pcb[i].pid == arg1) {
      wait(&pcb[i]);
      break;
    }
  }
  return 0;
}

static int _getpid(int arg1, int arg2, int arg3) {
  return current_running->pid;
}

static int _sleep(int arg1, int arg2, int arg3) {
  do_sleep((uint32_t)arg1);
  return 0;
}

static int _write(int arg1, int arg2, int arg3) {
  screen_write((char*)arg1);
  return 0;
}

static int _cursor(int arg1, int arg2, int arg3) {
  screen_move_cursor(arg1, arg2);
  return 0;
}

static int _reflush(int arg1, int arg2, int arg3) {
  screen_reflush();
  return 0;
}

static int _mutex_lock_init(int arg1, int arg2, int arg3) {
  do_mutex_lock_init((mutex_lock_t*)arg1);
  return 0;
}

static int _mutex_lock_acquire(int arg1, int arg2, int arg3) {
  do_mutex_lock_acquire((mutex_lock_t*)arg1);
  return 0;
}

static int _mutex_lock_release(int arg1, int arg2, int arg3) {
  do_mutex_lock_release((mutex_lock_t*)arg1);
  return 0;
}

static int _semaphore_init(int arg1, int arg2, int arg3) {
  return 0;
}

static int _semaphore_up(int arg1, int arg2, int arg3) {
  return 0;
}

static int _semaphore_down(int arg1, int arg2, int arg3) {
  return 0;
}

static int _condition_init(int arg1, int arg2, int arg3) {
  return 0;
}

static int _condition_wait(int arg1, int arg2, int arg3) {
  return 0;
}

static int _condition_signal(int arg1, int arg2, int arg3) {
  return 0;
}

static int _condition_broadcast(int arg1, int arg2, int arg3) {
  return 0;
}

static int _barrier_init(int arg1, int arg2, int arg3) {
  return 0;
}

static int _barrier_wait(int arg1, int arg2, int arg3) {
  return 0;
}

void init_syscall_table() {
#define def_syscall(x) syscall[syscall_##x] = _##x
  def_syscall(spawn);
  def_syscall(kill);
  def_syscall(exit);
  def_syscall(waitpid);
  def_syscall(getpid);
  def_syscall(sleep);
  def_syscall(write);
  def_syscall(cursor);
  def_syscall(reflush);
  def_syscall(mutex_lock_init);
  def_syscall(mutex_lock_acquire);
  def_syscall(mutex_lock_release);
  def_syscall(semaphore_init);
  def_syscall(semaphore_up);
  def_syscall(semaphore_down);
  def_syscall(condition_init);
  def_syscall(condition_wait);
  def_syscall(condition_signal);
  def_syscall(condition_broadcast);
  def_syscall(barrier_init);
  def_syscall(barrier_wait);
}

int system_call_helper()
{
  int fn, arg1, arg2, arg3;
  fn = current_running->context.regs[4];
  arg1 = current_running->context.regs[5];
  arg2 = current_running->context.regs[6];
  arg3 = current_running->context.regs[7];
  if (syscall[fn]) {
    uint32_t ret;
    ret = syscall[fn](arg1, arg2, arg3);
    current_running->context.regs[2] = ret;
  }
}

void sys_spawn(task_info_t* task) {
  invoke_syscall(syscall_spawn, (int)task, IGNORE, IGNORE);
}


void sys_kill(int pid) {
  invoke_syscall(syscall_kill, pid, IGNORE, IGNORE);
}

void sys_exit() {
  invoke_syscall(syscall_exit, IGNORE, IGNORE, IGNORE);
}

void sys_waitpid(int pid) {
  invoke_syscall(syscall_waitpid, pid, IGNORE, IGNORE);
}

int sys_getpid() {
  return invoke_syscall(syscall_getpid, IGNORE, IGNORE, IGNORE);
}

void sys_sleep(uint32_t time)
{
  invoke_syscall(syscall_sleep, time, IGNORE, IGNORE);
}

void sys_write(char *buff)
{
  invoke_syscall(syscall_write, (int)buff, IGNORE, IGNORE);
}

void sys_reflush()
{
  invoke_syscall(syscall_reflush, IGNORE, IGNORE, IGNORE);
}

void sys_move_cursor(int x, int y)
{
  invoke_syscall(syscall_cursor, x, y, IGNORE);
}

void mutex_lock_init(mutex_lock_t *lock)
{
  invoke_syscall(syscall_mutex_lock_init, (int)lock, IGNORE, IGNORE);
}

void mutex_lock_acquire(mutex_lock_t *lock)
{
  invoke_syscall(syscall_mutex_lock_acquire, (int)lock, IGNORE, IGNORE);
}

void mutex_lock_release(mutex_lock_t *lock)
{
  invoke_syscall(syscall_mutex_lock_release, (int)lock, IGNORE, IGNORE);
}

void semaphore_init(semaphore_t *sem, int init) {
}

void semaphore_up(semaphore_t *sem) {
}

void semaphore_down(semaphore_t *sem) {
}

void condition_init(condition_t *condition) {
}

void condition_wait(mutex_lock_t *lock, condition_t *condition) {
}

void condition_signal(condition_t *condition) {
}

void condition_broadcast(condition_t *condition) {
}

void barrier_init(barrier_t *bar, int num) {
}

void barrier_wait(barrier_t *bar) {
}
