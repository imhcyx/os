#include "fs.h"

uint32_t alloc_block() {
  int buf[1024];
  struct sb super;
  int ib, ibmax, i, j;
  int bunch;
  sdread(&super, OFF_SB, sizeof(super));
  ibmax = super.size/4096/8;
  for (ib=0; ib<ibmax; ib++) {
    sdread(buf, super.off_bitmap, 4096);
    for (i=0; i<1024; i++) {
      if (buf[i]==0xffffffff) continue;
      bunch = buf[i];
      for (j=0; j<32; j++) {
        if ((bunch&1)==0) {
          buf[i] |= 1<<j;
          sdwrite(buf, super.off_bitmap+ib*4096, 4096);
          super.usage += 4096;
          sdwrite(&super, OFF_SB, sizeof(super));
          return super.off_blocks+(ib*4096*8+i*32+j)*4096;
        }
        bunch >>= 1;
      }
    }
  }
  panic("disk full");
}

void free_block(uint32_t offset) {
  int buf[1024];
  struct sb super;
  int ib, i, j;
  sdread(&super, OFF_SB, sizeof(super));
  offset -= super.off_blocks;
  offset /= 4096;
  ib = offset/4096/8;
  offset -= ib*4096*8;
  i = offset/32;
  j = offset-i*32;
  sdread(buf, super.off_bitmap+ib*4096, 4096);
  buf[i] &= ~(1<<j);
  sdwrite(buf, super.off_bitmap+ib*4096, 4096);
  super.usage -= 4096;
  sdwrite(&super, OFF_SB, sizeof(super));
}

void fs_mkfs(uint32_t size) {
  int buf[1024];
  struct sb super;
  struct inode root;
  uint32_t off_root;
  int i;
  screen_clear();
  screen_move_cursor(0,0);
  printf("write superblock...\n");
  super.magic = FS_MAGIC;
  super.size = size;
  super.usage = 0;
  super.off_bitmap = OFF_SB+SIZE_SB;
  super.off_blocks = super.off_bitmap+size/4096/8;
  super.off_root = super.off_blocks;
  sdwrite(&super, OFF_SB, sizeof(super));
  printf("write bitmap...\n");
  memset(buf, 0, 4096);
  for (i=0; i<size/4096/8/4096; i++)
    sdwrite(buf, super.off_bitmap+i*4096, 4096);
  off_root = alloc_block();
  printf("size: %u\n", size);
  printf("bitmap offset: %08x\n", super.off_bitmap);
  printf("blocks offset: %08x\n", super.off_blocks);
  printf("root offset: %08x\n", off_root);
  // assert
  if (off_root!=super.off_root) {
    panic("unexpected root inode allocation");
  }
  memset(&root, 0, sizeof(root));
  root.type = INODE_DIR;
  sdwrite(&root, off_root, sizeof(root));
}

void fs_stat() {
  struct sb super;
  sdread(&super, OFF_SB, sizeof(super));
  screen_clear();
  screen_move_cursor(0,0);
  printf("magic: %08x\n", super.magic);
  if (super.magic!=FS_MAGIC) {
    printf("unknown file system\n");
    return;
  }
  printf("total size: %u\n", super.size);
  printf("used size: %u (%d%%)\n", super.usage, super.usage/(super.size/100));
  printf("bitmap offset: %08x\n", super.off_bitmap);
  printf("blocks offset: %08x\n", super.off_blocks);
}

int fs_open(char* name) {
}

void fs_read(int fd, char* buf, uint32_t size) {
}

void fs_write(int fd, char* buf, uint32_t size) {
}

void fs_close(int fd) {
}
