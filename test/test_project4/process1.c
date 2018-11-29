#include "sched.h"
#include "stdio.h"
#include "syscall.h"
#include "screen.h"
#include "test4.h"

static char blank[] = {"                   "};
static char plane1[] = {"    ___         _  "};
static char plane2[] = {"| __\\_\\______/_| "};
static char plane3[] = {"<[___\\_\\_______| "};
static char plane4[] = {"|  o'o             "};

void drawing_task1(void)
{
    int i = 22, j = 10;

    while (1)
    {
        for (i = 60; i > 0; i--)
        {
            disable_interrupt();
            /* move */
            vt100_move_cursor(i, j + 0);
            printk("%s", plane1);

            vt100_move_cursor(i, j + 1);
            printk("%s", plane2);

            vt100_move_cursor(i, j + 2);
            printk("%s", plane3);

            vt100_move_cursor(i, j + 3);
            printk("%s", plane4);
            enable_interrupt();
        }

        disable_interrupt();
        vt100_move_cursor(1, j + 0);
        printk("%s", blank);

        vt100_move_cursor(1, j + 1);
        printk("%s", blank);

        vt100_move_cursor(1, j + 2);
        printk("%s", blank);

        vt100_move_cursor(1, j + 3);
        printk("%s", blank);
        enable_interrupt();
    }
}
