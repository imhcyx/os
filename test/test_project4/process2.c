#include "sched.h"
#include "stdio.h"
#include "syscall.h"
#include "time.h"
#include "screen.h"
#include "test4.h"

#define RW_TIMES 2

int rand()
{	
	int current_time = get_timer();
	return current_time % 100000;
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

static void putchar(char c) {
  printf("%c", c);
}

static void delchar() {
  printf("\b \b");
}

static void scanf(int *mem)
{
  char ch;
  char buf[256];
  int len = 0;
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
  *mem = hex2u(buf);
}

void rw_task1(void)
{
	int mem1, mem2 = 0;
	int curs = 0;
	int memory[RW_TIMES];
	int i = 0;
	for(i = 0; i < RW_TIMES; i++)
	{
		screen_move_cursor(0, curs+i);
		scanf(&mem1);
		screen_move_cursor(0, curs+i);
		memory[i] = mem2 = rand();
		*(int *)mem1 = mem2;
		printf("Write: 0x%x, %d", mem1, mem2);
	}
	curs = RW_TIMES;
	for(i = 0; i < RW_TIMES; i++)
	{
		screen_move_cursor(0, curs+i);
		scanf(&mem1);
		screen_move_cursor(0, curs+i);
		memory[i+RW_TIMES] = *(int *)mem1;
		if(memory[i+RW_TIMES] == memory[i])
			printf("Read succeed: %d", memory[i+RW_TIMES]);
		else
			printf("Read error: %d", memory[i+RW_TIMES]);
	}
	while(1);
	//Only input address.
	//Achieving input r/w command is recommended but not required.
}
