/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *            Copyright (C) 2018 Institute of Computing Technology, CAS
 *               Author : Han Shukai (email : hanshukai@ict.ac.cn)
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *                  The shell acts as a task running in user mode. 
 *       The main function is to make system calls through the user's output.
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

#include "test.h"
#include "stdio.h"
#include "screen.h"
#include "syscall.h"
#include "stdarg.h"

static inline uint32_t get_cp0_status() {
  uint32_t status;
  __asm__ volatile (
    "mfc0 %0, $12\n"
    : "=r" (status)
  );
  return status;
}

static inline void set_cp0_status(uint32_t status) {
  __asm volatile (
    "mtc0 %0, $12\n"
    :: "r" (status)
  );
}

static void disable_interrupt()
{
    uint32_t cp0_status = get_cp0_status();
    cp0_status &= 0xfffffffe;
    set_cp0_status(cp0_status);
}

static void enable_interrupt()
{
    uint32_t cp0_status = get_cp0_status();
    cp0_status |= 0x01;
    set_cp0_status(cp0_status);
}

static char read_uart_ch(void)
{
    char ch = 0;
    unsigned char *read_port = (unsigned char *)(0xbfe48000 + 0x00);
    unsigned char *stat_port = (unsigned char *)(0xbfe48000 + 0x05);

    while ((*stat_port & 0x01))
    {
        ch = *read_port;
    }
    return ch;
}

struct task_info task1 = {"task1", (uint32_t)&ready_to_exit_task, USER_PROCESS};
struct task_info task2 = {"task2", (uint32_t)&wait_lock_task, USER_PROCESS};
struct task_info task3 = {"task3", (uint32_t)&wait_exit_task, USER_PROCESS};

struct task_info task4 = {"task4", (uint32_t)&semaphore_add_task1, USER_PROCESS};
struct task_info task5 = {"task5", (uint32_t)&semaphore_add_task2, USER_PROCESS};
struct task_info task6 = {"task6", (uint32_t)&semaphore_add_task3, USER_PROCESS};

struct task_info task7 = {"task7", (uint32_t)&producer_task, USER_PROCESS};
struct task_info task8 = {"task8", (uint32_t)&consumer_task1, USER_PROCESS};
struct task_info task9 = {"task9", (uint32_t)&consumer_task2, USER_PROCESS};

struct task_info task10 = {"task10", (uint32_t)&barrier_task1, USER_PROCESS};
struct task_info task11 = {"task11", (uint32_t)&barrier_task2, USER_PROCESS};
struct task_info task12 = {"task12", (uint32_t)&barrier_task3, USER_PROCESS};

struct task_info task13 = {"SunQuan",(uint32_t)&SunQuan, USER_PROCESS};
struct task_info task14 = {"LiuBei", (uint32_t)&LiuBei, USER_PROCESS};
struct task_info task15 = {"CaoCao", (uint32_t)&CaoCao, USER_PROCESS};

static struct task_info *test_tasks[16] = {&task1, &task2, &task3,
                                           &task4, &task5, &task6,
                                           &task7, &task8, &task9,
                                           &task10, &task11, &task12,
                                           &task13, &task14, &task15};
static int num_test_tasks = 15;

#define SHELL_Y 21
#define SHELL_HEIGHT (SCREEN_HEIGHT-SHELL_Y-1)

static char scrbuf[SHELL_HEIGHT][SCREEN_WIDTH+1]; // each line ends with '\0'
static int curline = 0, startline = 0;
static int cursor = 0;

static void flushline(int line) {
  int i;
  for (i=0; i<SCREEN_WIDTH; i++)
    scrbuf[line][i] = ' ';
  scrbuf[line][SCREEN_WIDTH] = '\0';
}

static void breakline() {
  int i;
  curline = (curline + 1) % SHELL_HEIGHT;
  if (startline == curline)
    startline = (curline + 1) % SHELL_HEIGHT;
  cursor = 0;
  flushline(curline);
}

static void backline() {
  if (startline != curline)
    curline = (curline + SHELL_HEIGHT - 1) % SHELL_HEIGHT;
  for (cursor=0; scrbuf[curline][cursor]; cursor++);
}

static void refresh() {
  int i, y;
  disable_interrupt();
  vt100_move_cursor(1, SHELL_Y);
  printk("-------------------- COMMAND --------------------");
  i = startline;
  y = SHELL_Y;
  do {
    vt100_move_cursor(1, 1+y++);
    printk(scrbuf[i]);
    i = (i+1) % SHELL_HEIGHT;
  } while (i != startline);
  enable_interrupt();
}

static void putchar(char c) {
  if (cursor >= SCREEN_WIDTH) breakline();
  if (c == '\n')
    breakline();
  else
    scrbuf[curline][cursor++] = c;
  refresh();
}

static void delchar() {
  if (cursor <= 0) backline();
  if (cursor > 0)
    scrbuf[curline][--cursor] = ' ';
  refresh();
}

static void printstr(const char *str) {
  while (*str) {
    if (cursor >= SCREEN_WIDTH) breakline();
    if (*str == '\n') {
      breakline();
      str++;
    }
    else {
      scrbuf[curline][cursor++] = *str++;
    }
  }
  refresh();
}

static int shell_printf(const char *fmt, ...) {
  int ret;
  va_list va;
  char buf[256];
  va_start(va, fmt);
  ret = mini_vsnprintf(buf, 256, fmt, va);
  va_end(va);
  buf[ret] = '\0';
  printstr(buf);
  return ret;
}

static void scrinit() {
  int i;
  for (i=0; i<SHELL_HEIGHT; i++)
    flushline(i);
  curline = 0;
  startline = 0;
  cursor = 0;
  refresh();
}

static void pslist() {
  char desc[] = {' ','I','B','R','R','E'};
  int i;
  for (i=0; i<NUM_MAX_TASK; i++) {
    if (pcb[i].status != TASK_UNUSED) {
      shell_printf("%c %d %s\n", desc[pcb[i].status], pcb[i].pid, pcb[i].name);
    }
  }
}

static char *tokenize(char *str, char *token, int size) {
  int i;
  i = 0;
  while (*str == ' ') str++;
  while (*str != '\0' && *str != ' ' && i<size-1) {
    token[i++] = *str++;
  }
  token[i] = '\0';
  return str;
}

// ignore overflow
static int myatoi(char *str) {
  int res = 0;
  int sgn = 1;
  while (*str == ' ') str++;
  if (*str == '+') str++;
  else if (*str == '-') {
    str++;
    sgn = -1;
  }
  while (*str >= '0' && *str <= '9') {
    res *= 10;
    res += *str++ - '0';
  }
  return res * sgn;
}

static uint32_t hex2u(char *str) {
  uint32_t res = 0;
  while (*str == ' ') str++;
  while (1) {
    if (*str >= '0' && *str <= '9') {
      res <<= 4;
      res |= *str++ - '0';
    }
    else if (*str >= 'a' && *str <= 'f') {
      res <<= 4;
      res |= *str++ - 'a' + 10;
    }
    else if (*str >= 'A' && *str <= 'F') {
      res <<= 4;
      res |= *str++ - 'A' + 10;
    }
    else {
      break;
    }
  }
  return res;
}

static void exec_command(char *cmd) {
  int i;
  char token[32];
  cmd = tokenize(cmd, token, sizeof(token));
  if (!strcmp(token, "clear")) {
    scrinit();
    return;
  }
  else if (!strcmp(token, "ps")) {
    pslist();
    return;
  }
  else if (!strcmp(token, "kill")) {
    cmd = tokenize(cmd, token, sizeof(token));
    if (!strcmp(token, "")) {
      shell_printf("missing parameter\n");
      return;
    }
    sys_kill(myatoi(token));
    return;
  }
  else if (!strcmp(token, "exec")) {
    cmd = tokenize(cmd, token, sizeof(token));
    if (!strcmp(token, "")) {
      shell_printf("missing parameter\n");
      return;
    }
    i = myatoi(token);
    if (i>=0&&i<num_test_tasks) {
      sys_spawn(test_tasks[i]);
      shell_printf("task %s created\n", test_tasks[i]->name);
    }
    return;
  }
  else if (!strcmp(token, "sr")) {
    // for debug only
    uint32_t val;
    cmd = tokenize(cmd, token, sizeof(token));
    if (!strcmp(token, "")) {
      shell_printf("sr: 0x%08x\n", get_cp0_status());
      return;
    }
    val = hex2u(token);
    shell_printf("set sr to 0x%08x\n", val);
    set_cp0_status(val);
    return;
  }
  else if (!strcmp(token, "")) {
    return;
  }
  else {
    shell_printf("unrecognized command %s\n", token);
    return;
  }
}

void test_shell()
{
  char ch;
  char buf[256];
  int len;
  scrinit();
  while (1)
  {
    printstr("UCAS>");
    len = 0;
    while (1) {
      ch = 0;
      while (!ch) {
        disable_interrupt();
        ch = read_uart_ch();
        enable_interrupt();
      } 
      if (ch == '\r') {
        putchar('\n');
        buf[len] = '\0';
        break;
      }
      else if (ch == '\x7f' || ch == '\b') {
        // \x7f and \b are backspace chars for qemu and minicom respectively
        if (len > 0) {
          len--;
          delchar();
        }
      }
      else {
        if (len < sizeof(buf)-1) {
          buf[len++] = ch;
          putchar(ch);
        }
      }
    }
    exec_command(buf);
#if 0
    // debug
    {
      int i;
      for (i=0; i<SHELL_HEIGHT; i++) {
        vt100_move_cursor(1, i+1);
        printk(scrbuf[i]);
      }
      vt100_move_cursor(1, 12);
      printk("%d %d", curline, startline);
    }
#endif
  }
}
