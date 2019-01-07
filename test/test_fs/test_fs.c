#include "stdio.h"
#include "string.h"
#include "test_fs.h"
#include "syscall.h"

static char buff[64];

void test_fs(void)
{
    int i, j;
    int fd = sys_fopen("1.txt");

    for (i = 0; i < 10; i++)
    {
        sys_fwrite(fd, "hello world!\n", 13);
    }

    for (i = 0; i < 10; i++)
    {
        sys_fread(fd, buff, 13);
        for (j = 0; j < 13; j++)
        {
            printf("%c", buff[j]);
        }
    }

    sys_close(fd);
    sys_exit();
}

void test_largefs() {
  uint32_t offset, data;
  int fd = sys_fopen("largefile");
  screen_clear();
  screen_move_cursor(0, 0);
  for (offset=0; offset<(1<<30); offset+=(1<<28)) {
    data = (get_timer()^0x12345678)*12345678;
    printf("write offset %u: %08x\n", offset, data);
    sys_seek(fd, offset);
    sys_fwrite(fd, &data, 4);
  }
  for (offset=0; offset<(1<<30); offset+=(1<<28)) {
    sys_seek(fd, offset);
    sys_fread(fd, &data, 4);
    printf("read offset %u: %08x\n", offset, data);
  }
  sys_close(fd);
  sys_exit();
}
