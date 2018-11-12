/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *            Copyright (C) 2018 Institute of Computing Technology, CAS
 *               Author : Han Shukai (email : hanshukai@ict.ac.cn)
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *         The kernel's entry, where most of the initialization work is done.
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this 
 * software and associated documentation files (the "Software"), to deal in the Software 
 * without restriction, including without limitation the rights to use, copy, modify, 
 * merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit 
 * persons to whom the Software is furnished to do so, subject to the following conditions:
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

#include "irq.h"
#include "test.h"
#include "stdio.h"
#include "sched.h"
#include "screen.h"
#include "common.h"
#include "syscall.h"

static void init_pcb() {
  sched_init();
}

static void init_exception_handler()
{
  char *src = (char*)exception_handler_begin;
  char *dst = (char*)0x80000180;
  while (src < (char*)exception_handler_end)
    *dst++ = *src++;
}

static void init_exception()
{
	// 1. Get CP0_STATUS
	// 2. Disable all interrupt
	// 3. Copy the level 2 exception handling code to 0x80000180
	// 4. reset CP0_COMPARE & CP0_COUNT register
  // disable interrupts
  __asm__ volatile (
    "mfc0 $t0, $12\n"
    "and $t0, 0xfffffffe\n"
    "mtc0 $t0, $12\n"
    ::: "t0"
  );
  // copy exception handler
  init_exception_handler();
  // set CP0_COMPARE & CP0_COUNT
  __asm__ volatile (
    "li $t0, 1000\n"
    "mtc0 $t0, $11\n"
    "mtc0 $zero, $9\n"
    ::: "t0"
  );
}

static void init_syscall(void)
{
	// init system call table.
  init_syscall_table();
}

// jump from bootloader.
// The beginning of everything >_< ~~~~~~~~~~~~~~
void __attribute__((section(".entry_function"))) _start(void)
{
	// Close the cache, no longer refresh the cache 
	// when making the exception vector entry copy
	asm_start();

	// init interrupt (^_^)
	init_exception();
	printk("> [INIT] Interrupt processing initialization succeeded.\n");

	// init system call table (0_0)
	init_syscall();
	printk("> [INIT] System call initialized successfully.\n");

	// init Process Control Block (-_-!)
	init_pcb();
	printk("> [INIT] PCB initialization succeeded.\n");

	// init screen (QAQ)
	init_screen();
	//printk("> [INIT] SCREEN initialization succeeded.\n");

	// enable interrupt and set BEV=0
  __asm__ volatile (
    "mfc0 $t0, $12\n"
    "and $t0, 0xffbfffff\n"
    "or $t0, 0xff01\n"
    "mtc0 $t0, $12\n"
    ::: "t0"
  );
	
	while (1)
	{
    struct task_info shell = {"shell", (uint32_t)&test_shell, USER_PROCESS, 10};
    pcb_t *proc = spawn(&shell);
    // wait must be called in interrupt context
    sys_waitpid(proc->pid);
	};
	return;
}
