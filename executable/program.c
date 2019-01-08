enum {

  syscall_sleep=5,

  syscall_write=6,
  syscall_cursor=7,
  syscall_reflush=8,
  syscall_clear=9

};

#define IGNORE 0

void invoke_syscall(int sys, int a1, int a2, int a3);

void sys_sleep(unsigned int time)
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

int main() {
  int i=0;
  char buf[] = "Hello World! .";
  char c[] = {'-','\\','|','/'};
  while (1) {
    buf[13] = c[i++%4];
    sys_move_cursor(5,0);
    sys_write(buf);
    sys_sleep(1);
  }
  return 0;
}
