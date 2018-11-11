#ifndef INCLUDE_STDIO_H_
#define INCLUDE_STDIO_H_

/* kernel printf */
int printk(const char *fmt, ...);
int printk_buffered(const char *fmt, ...);

/* user printk */
int printf(const char *fmt, ...);

#endif
