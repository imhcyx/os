#include "lock.h"
#include "sched.h"
#include "common.h"
#include "screen.h"
#include "syscall.h"

static int _spawn(int arg1, int arg2, int arg3) {
  spawn((void*)arg1, arg2);
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

static int _clear(int arg1, int arg2, int arg3) {
  screen_clear();
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
  do_semaphore_init((semaphore_t*)arg1, arg2);
  return 0;
}

static int _semaphore_up(int arg1, int arg2, int arg3) {
  do_semaphore_up((semaphore_t*)arg1);
  return 0;
}

static int _semaphore_down(int arg1, int arg2, int arg3) {
  do_semaphore_down((semaphore_t*)arg1);
  return 0;
}

static int _condition_init(int arg1, int arg2, int arg3) {
  do_condition_init((condition_t*)arg1);
  return 0;
}

static int _condition_wait(int arg1, int arg2, int arg3) {
  do_condition_wait((mutex_lock_t*)arg1, (condition_t*)arg2);
  return 0;
}

static int _condition_signal(int arg1, int arg2, int arg3) {
  do_condition_signal((condition_t*)arg1);
  return 0;
}

static int _condition_broadcast(int arg1, int arg2, int arg3) {
  do_condition_broadcast((condition_t*)arg1);
  return 0;
}

static int _barrier_init(int arg1, int arg2, int arg3) {
  do_barrier_init((barrier_t*)arg1, arg2);
  return 0;
}

static int _barrier_wait(int arg1, int arg2, int arg3) {
  do_barrier_wait((barrier_t*)arg1);
  return 0;
}

static int _init_mac(int arg1, int arg2, int arg3) {
  do_init_mac();
  return 0;
}

static int _net_send(int arg1, int arg2, int arg3) {
  do_net_send(arg1, arg2);
  return 0;
}

static int _net_recv(int arg1, int arg2, int arg3) {
  return do_net_recv(arg1, arg2, arg3);
}

static int _wait_recv_package(int arg1, int arg2, int arg3) {
  do_wait_recv_package();
  return 0;
}

static int _fopen(int arg1, int arg2, int arg3) {
  return fs_open((char*)arg1);
}

static int _fread(int arg1, int arg2, int arg3) {
  fs_read(arg1, (char*)arg2, arg3);
  return 0;
}

static int _fwrite(int arg1, int arg2, int arg3) {
  fs_write(arg1, (char*)arg2, arg3);
  return 0;
}

static int _close(int arg1, int arg2, int arg3) {
  fs_close(arg1);
  return 0;
}

static int _seek(int arg1, int arg2, int arg3) {
  fs_seek(arg1, (uint32_t)arg2);
  return 0;
}

static int _cwd(int arg1, int arg2, int arg3) {
  fs_cwd((char*)arg1);
  return 0;
}

static int _pwd(int arg1, int arg2, int arg3) {
  fs_pwd((char*)arg1);
  return 0;
}

static int _mkdir(int arg1, int arg2, int arg3) {
  fs_mkdir((char*)arg1);
  return 0;
}

static int _unlink(int arg1, int arg2, int arg3) {
  fs_unlink((char*)arg1);
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
  def_syscall(clear);
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
  def_syscall(init_mac);
  def_syscall(net_send);
  def_syscall(net_recv);
  def_syscall(wait_recv_package);
  def_syscall(fopen);
  def_syscall(fread);
  def_syscall(fwrite);
  def_syscall(close);
  def_syscall(seek);
  def_syscall(cwd);
  def_syscall(pwd);
  def_syscall(mkdir);
  def_syscall(unlink);
}

int system_call_helper()
{
  pcb_t *proc = current_running;
  int fn, arg1, arg2, arg3;
  fn = proc->context.regs[4];
  arg1 = proc->context.regs[5];
  arg2 = proc->context.regs[6];
  arg3 = proc->context.regs[7];
  if (syscall[fn]) {
    uint32_t ret;
    ret = syscall[fn](arg1, arg2, arg3);
    // DO NOT use current_running since the task may be switched
    proc->context.regs[2] = ret;
    proc->context.cp0_epc += 4;
  }
}

void sys_spawn(void* task, int isfile) {
  invoke_syscall(syscall_spawn, (int)task, isfile, IGNORE);
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

void sys_clear()
{
  invoke_syscall(syscall_clear, IGNORE, IGNORE, IGNORE);
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
  invoke_syscall(syscall_semaphore_init, (int)sem, init, IGNORE);
}

void semaphore_up(semaphore_t *sem) {
  invoke_syscall(syscall_semaphore_up, (int)sem, IGNORE, IGNORE);
}

void semaphore_down(semaphore_t *sem) {
  invoke_syscall(syscall_semaphore_down, (int)sem, IGNORE, IGNORE);
}

void condition_init(condition_t *condition) {
  invoke_syscall(syscall_condition_init, (int)condition, IGNORE, IGNORE);
}

void condition_wait(mutex_lock_t *lock, condition_t *condition) {
  invoke_syscall(syscall_condition_wait, (int)lock, (int)condition, IGNORE);
}

void condition_signal(condition_t *condition) {
  invoke_syscall(syscall_condition_signal, (int)condition, IGNORE, IGNORE);
}

void condition_broadcast(condition_t *condition) {
  invoke_syscall(syscall_condition_broadcast, (int)condition, IGNORE, IGNORE);
}

void barrier_init(barrier_t *bar, int num) {
  invoke_syscall(syscall_barrier_init, (int)bar, num, IGNORE);
}

void barrier_wait(barrier_t *bar) {
  invoke_syscall(syscall_barrier_wait, (int)bar, IGNORE, IGNORE);
}

void sys_init_mac() {
  invoke_syscall(syscall_init_mac, IGNORE, IGNORE, IGNORE);
}

void sys_net_send(uint32_t td, uint32_t td_phy) {
  invoke_syscall(syscall_net_send, (int)td, (int) td_phy, IGNORE);
}

uint32_t sys_net_recv(uint32_t rd, uint32_t rd_phy, uint32_t daddr) {
  return invoke_syscall(syscall_net_recv, (int)rd, (int)rd_phy, (int)daddr);
}

void sys_wait_recv_package() {
  invoke_syscall(syscall_wait_recv_package, IGNORE, IGNORE, IGNORE);
}

int sys_fopen(char *name) {
  return invoke_syscall(syscall_fopen, (int)name, IGNORE, IGNORE);
}

void sys_fread(int fd, void *buf, uint32_t size) {
  invoke_syscall(syscall_fread, fd, (int)buf, (int)size);
}

void sys_fwrite(int fd, void *buf, uint32_t size) {
  invoke_syscall(syscall_fwrite, fd, (int)buf, (int)size);
}

void sys_close(int fd) {
  invoke_syscall(syscall_close, fd, IGNORE, IGNORE);
}

void sys_seek(int fd, uint32_t offset) {
  invoke_syscall(syscall_seek, fd, (int)offset, IGNORE);
}

void sys_cwd(char *name) {
  invoke_syscall(syscall_cwd, (int)name, IGNORE, IGNORE);
}

void sys_pwd(char *name) {
  invoke_syscall(syscall_pwd, (int)name, IGNORE, IGNORE);
}

void sys_mkdir(char *name) {
  invoke_syscall(syscall_mkdir, (int)name, IGNORE, IGNORE);
}

void sys_unlink(char *name) {
  invoke_syscall(syscall_unlink, (int)name, IGNORE, IGNORE);
}
