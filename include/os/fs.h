#ifndef _FS_H
#define _FS_H

#include "type.h"

#define OFF_SB 0x100000
#define SIZE_SB 0x100000
#define FS_MAGIC 0xfedcba98

struct sb {
  uint32_t magic;
  uint32_t size; // in bytes
  uint32_t usage; // in bytes
  uint32_t off_bitmap; // absolute
  uint32_t off_blocks; // absolute
  uint32_t off_root; // absolute
};

enum {
  INODE_DIR,
  INODE_FILE
};

// for both file and directory
// currently only up to 8 files in a directory supported using 8 direct pointers
struct inode {
  int type;
  uint32_t size; // for file only
  uint32_t parent; // absolute
  uint32_t direct[8]; // absolute
  uint32_t indirect; // absolute
  uint32_t indirect2; // absolute
  char name[12];
};

struct fd {
  uint32_t inode; // absolute, 0 if unused
  uint32_t readoffset; // relative to file
  uint32_t writeoffset; // relative to file
  uint32_t size;
};

int fs_open(char*);
void fs_read(int, char*, uint32_t);
void fs_write(int, char*, uint32_t);
void fs_close(int);

#endif
