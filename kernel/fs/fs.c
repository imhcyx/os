#include "fs.h"
#include "sched.h"

uint32_t alloc_block() {
  int buf[1024];
  struct sb super;
  int ib, ibmax, i, j;
  int bunch;
  // read superblock
  sdread(&super, OFF_SB, sizeof(super));
  ibmax = super.size/4096/8;
  // find a zero bit in bitmap
  for (ib=0; ib<ibmax; ib++) {
    // read a block each time
    sdread(buf, super.off_bitmap, 4096);
    for (i=0; i<1024; i++) {
      // do 32-bit coarse search for acceleration
      if (buf[i]==0xffffffff) continue;
      bunch = buf[i];
      for (j=0; j<32; j++) {
        // do 1-bit fine search
        if ((bunch&1)==0) {
          // found, set the bit and update usage
          buf[i] |= 1<<j;
          sdwrite(buf, super.off_bitmap+ib*4096, 4096);
          super.usage += 4096;
          sdwrite(&super, OFF_SB, sizeof(super));
          // return absolute offset
          return super.off_blocks+(ib*4096*8+i*32+j)*4096;
        }
        bunch >>= 1;
      }
    }
  }
  // not found, panic here
  panic("disk full");
}

void free_block(uint32_t offset) {
  int buf[1024];
  struct sb super;
  int ib, i, j;
  if (offset == 0) return;
  // read superblock
  sdread(&super, OFF_SB, sizeof(super));
  // calculate the bit location in bitmap
  offset -= super.off_blocks;
  offset /= 4096;
  ib = offset/4096/8;
  offset -= ib*4096*8;
  i = offset/32;
  j = offset-i*32;
  // read & write a block each time
  sdread(buf, super.off_bitmap+ib*4096, 4096);
  buf[i] &= ~(1<<j);
  sdwrite(buf, super.off_bitmap+ib*4096, 4096);
  // update usage
  super.usage -= 4096;
  sdwrite(&super, OFF_SB, sizeof(super));
}

static uint32_t get_root() {
  struct sb super;
  sdread(&super, OFF_SB, sizeof(super));
  return super.off_root;
}

#define ensure_cwd() \
  if (!current_running->curdir) current_running->curdir=get_root()

static uint32_t parse_path(char *path) {
  uint32_t ino = current_running->curdir;
  char name[16], *pname;
  struct inode inode, inodechild;
  int i, found;
  // if path starts with '/', find from root node
  if (*path == '/') {
    path++;
    ino = get_root();
  }
  while (*path && ino) {
    // fail if current node is not a directory
    sdread(&inode, ino, sizeof(inode));
    if (inode.type != INODE_DIR) return 0;
    // skip leading '/'s
    while (*path == '/') path++;
    // copy a name between '/'s
    pname = name;
    while (*path && *path != '/') *pname++ = *path++;
    *pname = '\0';
    // do nothing if name is "."
    if (!strcmp(name, "."))
      continue;
    // go to parent if name is ".."
    else if (!strcmp(name, "..")) {
      ino = inode.parent;
    }
    // otherwise find the entry with the same name in current directory
    else {
      found = 0;
      for (i=0; i<8; i++) {
        if (inode.direct[i] == 0) continue;
        sdread(&inodechild, inode.direct[i], sizeof(inodechild));
        if (!strcmp(name, inodechild.name)) {
          ino = inode.direct[i];
          found = 1;
          break;
        }
      }
      if (!found) return 0;
    }
  }
  return ino;
}

static uint32_t find_inode(char *name, int remove) {
  struct inode inode, inodechild;
  int i;
  uint32_t ino;
  sdread(&inode, current_running->curdir, sizeof(inode));
  for (i=0; i<8; i++) {
    if (inode.direct[i] == 0) continue;
    sdread(&inodechild, inode.direct[i], sizeof(inodechild));
    if (!strcmp(name, inodechild.name)) {
      ino = inode.direct[i];
      if (remove) {
        inode.direct[i] = 0;
        sdwrite(&inode, current_running->curdir, sizeof(inode));
      }
      return ino;
    }
  }
  return 0;
}

static uint32_t create_inode(char *name, int type) {
  struct inode inode, inodechild;
  int i;
  sdread(&inode, current_running->curdir, sizeof(inode));
  for (i=0; i<8; i++) {
    if (inode.direct[i] == 0) continue;
    sdread(&inodechild, inode.direct[i], sizeof(inodechild));
    if (!strcmp(name, inodechild.name)) {
      if (type == inodechild.type)
        return inode.direct[i];
      else
        return 0;
    }
  }
  for (i=0; i<8; i++) {
    if (inode.direct[i] == 0) {
      uint32_t ino = alloc_block();
      inode.direct[i] = ino;
      sdwrite(&inode, current_running->curdir, sizeof(inode));
      memset(&inodechild, 0, sizeof(inodechild));
      inodechild.type = type;
      inodechild.parent = current_running->curdir;
      strcpy(inodechild.name, name);
      sdwrite(&inodechild, ino, sizeof(inodechild));
      return ino;
    }
  }
  panic("out of directory entries");
}

void fs_mkfs(uint32_t size) {
  int buf[1024];
  struct sb super;
  struct inode root;
  uint32_t off_root;
  int i;
  // calculate offsets and write superblock
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
  // clear bitmap
  printf("write bitmap...\n");
  memset(buf, 0, 4096);
  for (i=0; i<size/4096/8/4096; i++)
    sdwrite(buf, super.off_bitmap+i*4096, 4096);
  off_root = alloc_block();
  printf("size: %u\n", size);
  printf("bitmap offset: 0x%08x\n", super.off_bitmap);
  printf("blocks offset: 0x%08x\n", super.off_blocks);
  printf("root offset: 0x%08x\n", off_root);
  // assert allocated root node is at the desired location
  if (off_root!=super.off_root) {
    panic("unexpected root inode allocation");
  }
  // set up root node
  memset(&root, 0, sizeof(root));
  root.type = INODE_DIR;
  sdwrite(&root, off_root, sizeof(root));
}

void fs_stat() {
  struct sb super;
  sdread(&super, OFF_SB, sizeof(super));
  screen_clear();
  screen_move_cursor(0,0);
  printf("magic: 0x%08x\n", super.magic);
  if (super.magic!=FS_MAGIC) {
    printf("unknown file system\n");
    return;
  }
  printf("total size: %u\n", super.size);
  printf("used size: %u (%d%%)\n", super.usage, super.usage/(super.size/100));
  printf("bitmap offset: 0x%08x\n", super.off_bitmap);
  printf("blocks offset: 0x%08x\n", super.off_blocks);
}

static int alloc_fd() {
  int i;
  for (i=0; i<PCB_MAX_FD; i++)
    if (current_running->fd[i].inode == 0)
      return i;
  return -1;
}

int fs_open(char* name) {
  int fd;
  uint32_t ino;
  struct inode inode;
  ensure_cwd();
  ino = create_inode(name, INODE_FILE);
  if (!ino) return -1;
  sdread(&inode, ino, sizeof(inode));
  fd = alloc_fd();
  if (fd<0) return -1;
  current_running->fd[fd].inode = ino;
  current_running->fd[fd].readoffset = 0;
  current_running->fd[fd].writeoffset = 0;
  current_running->fd[fd].size = inode.size;
  return fd;
}

static uint32_t locate_file_datablock(uint32_t ino, uint32_t offset) {
  struct inode inode;
  uint32_t block[1024];
  uint32_t sdloc, indirect;
  sdread(&inode, ino, sizeof(inode));
  // direct
  if (offset <= 8*4096) {
    sdloc = inode.direct[offset/4096];
    if (!sdloc) {
      sdloc = alloc_block();
      inode.direct[offset/4096] = sdloc;
      sdwrite(&inode, ino, sizeof(inode));
      memset(block, 0, sizeof(block));
      sdwrite(block, sdloc, sizeof(block));
    }
    return sdloc;
  }
  // indirect
  offset -= 8*4096;
  if (offset <= 1024*4096) {
    if (!inode.indirect) {
      inode.indirect = alloc_block();
      sdwrite(&inode, ino, sizeof(inode));
      memset(block, 0, sizeof(block));
      sdwrite(block, inode.indirect, sizeof(block));
    }
    else {
      sdread(block, inode.indirect, sizeof(block));
    }
    sdloc = block[offset/4096];
    if (!sdloc) {
      sdloc = alloc_block();
      block[offset/4096] = sdloc;
      sdwrite(block, inode.indirect, sizeof(block));
      memset(block, 0, sizeof(block));
      sdwrite(block, sdloc, sizeof(block));
    }
    return sdloc;
  }
  // double indirect
  offset -= 1024*4096;
  if (!inode.indirect2) {
    inode.indirect2 = alloc_block();
    sdwrite(&inode, ino, sizeof(inode));
    memset(block, 0, sizeof(block));
    sdwrite(block, inode.indirect2, sizeof(block));
  }
  else {
    sdread(block, inode.indirect2, sizeof(block));
  }
  indirect = block[offset/(1024*4096)];
  if (!indirect) {
    indirect = alloc_block();
    block[offset/(1024*4096)] = indirect;
    sdwrite(block, inode.indirect2, sizeof(block));
    memset(block, 0, sizeof(block));
    sdwrite(block, indirect, sizeof(block));
  }
  else {
    sdread(block, indirect, sizeof(block));
  }
  offset %= 1024*4096;
  sdloc = block[offset/4096];
  if (!sdloc) {
    sdloc = alloc_block();
    block[offset/4096] = sdloc;
    sdwrite(block, indirect, sizeof(block));
    memset(block, 0, sizeof(block));
    sdwrite(block, sdloc, sizeof(block));
  }
  return sdloc;
  return 0;
}

void fs_read(int fd, char* buf, uint32_t size) {
  struct fd *pfd = &current_running->fd[fd];
  uint32_t current = pfd->readoffset;
  pfd->readoffset += size;
  uint32_t end = pfd->readoffset;
  uint32_t block;
  uint32_t bunchsize;
  while (current<end) {
    block = locate_file_datablock(pfd->inode, current);
    bunchsize = (current & ~4095) + 4096 - current;
    if (current+bunchsize>end) bunchsize = end-current;
    sdread(buf, block+(current%4096), bunchsize);
    buf += bunchsize;
    current += bunchsize;
  }
  if (current > pfd->size) pfd->size = current;
}

void fs_write(int fd, char* buf, uint32_t size) {
  struct fd *pfd = &current_running->fd[fd];
  uint32_t current = pfd->writeoffset;
  pfd->writeoffset += size;
  uint32_t end = pfd->writeoffset;
  uint32_t block;
  uint32_t bunchsize;
  char buffer[4096];
  while (current<end) {
    block = locate_file_datablock(pfd->inode, current);
    bunchsize = (current & ~4095) + 4096 - current;
    if (current+bunchsize>end) bunchsize = end-current;
    if ((current&4095)!=0 || bunchsize<4096) {
      sdread(buffer, block, 4096);
      memcpy(buffer+(current%4096), buf, bunchsize);
      sdwrite(buffer, block, 4096);
    }
    else {
      sdwrite(buf, block, 4096);
    }
    buf += bunchsize;
    current += bunchsize;
  }
  if (current > pfd->size) pfd->size = current;
}

void fs_close(int fd) {
  uint32_t ino = current_running->fd[fd].inode;
  struct inode inode;
  sdread(&inode, ino, sizeof(inode));
  inode.size = current_running->fd[fd].size;
  sdwrite(&inode, ino, sizeof(inode));
  current_running->fd[fd].inode = 0;
}

void fs_seek(int fd, uint32_t offset) {
  current_running->fd[fd].readoffset = offset;
  current_running->fd[fd].writeoffset = offset;
}

void fs_cwd(char *path) {
  struct inode inode;
  uint32_t ino;
  ensure_cwd();
  ino = parse_path(path);
  sdread(&inode, ino, sizeof(inode));
  if (inode.type == INODE_DIR)
    current_running->curdir = ino;
}

void fs_pwd(char *path) {
  struct sb super;
  struct inode inode;
  uint32_t ino;
  char names[16][16], *psrc;
  int count;
  sdread(&super, OFF_SB, sizeof(super));
  if (super.magic!=FS_MAGIC) {
    *path++ = '?';
    *path++ = '\0';
    return;
  }
  ensure_cwd();
  ino = current_running->curdir;
  count = 0;
  while (ino) {
    sdread(&inode, ino, sizeof(inode));
    strcpy(names[count++], inode.name);
    ino = inode.parent;
  }
  while (count) {
    psrc = names[--count];
    while (*psrc) *path++ = *psrc++;
    *path++ = '/';
  }
  *path++ = '\0';
}

void fs_mkdir(char *name) {
  ensure_cwd();
  create_inode(name, INODE_DIR);
}

static void remove_datablock(uint32_t offset, int indirect) {
  uint32_t block[1024];
  int i;
  if (offset == 0) return;
  if (indirect>0) {
    sdread(block, offset, sizeof(block));
    for (i=0; i<1024; i++)
      remove_datablock(block[i], indirect-1);
  }
  free_block(offset);
}

static void remove_inode(uint32_t ino) {
  struct inode inode;
  int i;
  if (ino == 0) return;
  sdread(&inode, ino, sizeof(inode));
  if (inode.type == INODE_DIR) {
    for (i=0; i<8; i++)
      remove_inode(inode.direct[i]);
  }
  else {
    for (i=0; i<8; i++)
      remove_datablock(inode.direct[i], 0);
    remove_datablock(inode.indirect, 1);
    remove_datablock(inode.indirect2, 2);
  }
  free_block(ino);
}

void fs_unlink(char *name) {
  ensure_cwd();
  remove_inode(find_inode(name, 1));
}

void fs_list(char *path) {
  uint32_t ino;
  struct inode inode, inodechild;
  int i;
  ensure_cwd();
  ino = current_running->curdir;
  if (path) {
    ino = parse_path(path);
    if (ino == 0) {
      shell_printf("%s not found\n", path);
      return;
    }
    sdread(&inode, ino, sizeof(inode));
    if (inode.type != INODE_DIR) {
      shell_printf("%s %u %s\n", "FILE", inode.size, inode.name);
      return;
    }
  }
  sdread(&inode, ino, sizeof(inode));
  for (i=0; i<8; i++)
    if (inode.direct[i]) {
      sdread(&inodechild, inode.direct[i], sizeof(inodechild));
      shell_printf("%s %u %s\n", inodechild.type == INODE_DIR ? "DIR " : "FILE", inodechild.size, inodechild.name);
    }
}
