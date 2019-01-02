/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *            Copyright (C) 2018 Institute of Computing Technology, CAS
 *               Author : Han Shukai (email : hanshukai@ict.ac.cn)
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *                       System call related processing
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE. 
 * 
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * */

#ifndef INCLUDE_SYSCALL_H_
#define INCLUDE_SYSCALL_H_

#include "type.h"
#include "sync.h"
#include "queue.h"

#define IGNORE 0

enum {

  syscall_spawn,
  syscall_kill,
  syscall_exit,
  syscall_waitpid,
  syscall_getpid,

  syscall_sleep,

  syscall_write,
  syscall_cursor,
  syscall_reflush,
  syscall_clear,

  syscall_mutex_lock_init,
  syscall_mutex_lock_acquire,
  syscall_mutex_lock_release,

  syscall_semaphore_init,
  syscall_semaphore_up,
  syscall_semaphore_down,

  syscall_condition_init,
  syscall_condition_wait,
  syscall_condition_signal,
  syscall_condition_broadcast,

  syscall_barrier_init,
  syscall_barrier_wait,

  syscall_init_mac,
  syscall_net_send,
  syscall_net_recv,
  syscall_wait_recv_package,

  syscall_fopen,
  syscall_fread,
  syscall_fwrite,
  syscall_close,

  NUM_SYSCALLS

};

/* syscall function pointer */
int (*syscall[NUM_SYSCALLS])(int, int, int);

void init_syscall_table();

int system_call_helper();
extern int invoke_syscall(int, int, int, int);

void sys_spawn(task_info_t*);
void sys_kill(int);
void sys_exit();
void sys_waitpid(int);
int sys_getpid();

void sys_sleep(uint32_t);

void sys_write(char *);
void sys_move_cursor(int, int);
void sys_reflush();

void mutex_lock_init(mutex_lock_t *);
void mutex_lock_acquire(mutex_lock_t *);
void mutex_lock_release(mutex_lock_t *);

void semaphore_init(semaphore_t *, int);
void semaphore_up(semaphore_t *);
void semaphore_down(semaphore_t *);

void condition_init(condition_t *condition);
void condition_wait(mutex_lock_t *lock, condition_t *condition);
void condition_signal(condition_t *condition);
void condition_broadcast(condition_t *condition);

void barrier_init(barrier_t *, int);
void barrier_wait(barrier_t *);

void sys_init_mac();
void sys_net_send(uint32_t td, uint32_t td_phy);
uint32_t sys_net_recv(uint32_t rd, uint32_t rd_phy, uint32_t daddr);
void sys_wait_recv_package();

int sys_fopen(char*);
void sys_fread(int, void*, uint32_t);
void sys_fwrite(int, void*, uint32_t);
void sys_close(int);

#endif
