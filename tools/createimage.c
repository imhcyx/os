#include <assert.h>
#include <elf.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SECTOR_SIZE 512

void write_bootblock(FILE *image, FILE *bbfile, Elf32_Phdr *Phdr);
Elf32_Phdr *read_exec_file(FILE *opfile);
uint8_t count_kernel_sectors(Elf32_Phdr *Phdr);
void extend_opt(Elf32_Phdr *Phdr_bb, Elf32_Phdr *Phdr_k, int kernelsz);

// Note:  1. returned pointer is malloc'ed
//        2. only reads one program header 
Elf32_Phdr *read_exec_file(FILE *opfile)
{
  Elf32_Ehdr ehdr;
  Elf32_Phdr *pphdr;

  // rewind file stream
  fseek(opfile, 0, SEEK_SET);
  // read ehdr
  fread(&ehdr, sizeof(ehdr), 1, opfile);
  // seek program header
  fseek(opfile, ehdr.e_phoff, SEEK_SET);
  // malloc return pointer
  pphdr = (Elf32_Phdr*)malloc(sizeof(*pphdr));
  if (!pphdr) return NULL;
  // read phdr
  fread(pphdr, sizeof(*pphdr), 1, opfile);

  return pphdr;
}

uint8_t count_kernel_sectors(Elf32_Phdr *Phdr)
{
  return (Phdr->p_memsz + SECTOR_SIZE - 1) / SECTOR_SIZE;
}

void write_bootblock(FILE *image, FILE *file, Elf32_Phdr *phdr)
{
  char buffer[SECTOR_SIZE];

  memset(buffer, 0, SECTOR_SIZE);
  // seek program code
  fseek(file, phdr->p_offset, SEEK_SET);
  // read program code
  fread(buffer, 1, phdr->p_filesz, file);
  // rewind image file
  fseek(image, 0, SEEK_SET);
  // write bootblock
  fwrite(buffer, 1, SECTOR_SIZE, image);
}

void write_kernel(FILE *image, FILE *knfile, Elf32_Phdr *Phdr, int kernelsz)
{
  char buffer[SECTOR_SIZE];
  int leftsize = Phdr->p_filesz;

  // seek kernel code position in image file
  fseek(image, SECTOR_SIZE, SEEK_SET);
  // seek program code in kernel file
  fseek(knfile, Phdr->p_offset, SEEK_SET);
  // write all kernel sectors
  while (kernelsz--) {
    memset(buffer, 0, SECTOR_SIZE);
    fread(buffer, 1, (leftsize < SECTOR_SIZE ? leftsize : SECTOR_SIZE), knfile);
    fwrite(buffer, 1, SECTOR_SIZE, image);
    leftsize -= SECTOR_SIZE;
    if (leftsize <= 0) break;
  }
  // padding with zero
  memset(buffer, 0, SECTOR_SIZE);
  while (kernelsz--) {
    fwrite(buffer, 1, SECTOR_SIZE, image);
  }
}

void record_kernel_sectors(FILE *image, uint8_t kernelsz)
{
  uint16_t ksize = kernelsz * SECTOR_SIZE;
  uint16_t sector_magic = 0xaa55;

  // seek end of bootblock
  fseek(image, SECTOR_SIZE - 4, SEEK_SET);
  // write kernel size
  fwrite(&ksize, 1, 2, image);
  // write magic number
  fwrite(&sector_magic, 1, 2, image);
}

void extend_opt(Elf32_Phdr *Phdr_bb, Elf32_Phdr *Phdr_k, int kernelsz)
{
  printf("bootblock size: 0x%x\n", Phdr_bb->p_filesz);
  printf("bootblock offset: 0x%x\n", Phdr_bb->p_offset);
  printf("kernel size: 0x%x\n", Phdr_k->p_filesz);
  printf("kernel offset: 0x%x\n", Phdr_k->p_offset);
  printf("kernel sectors: %d\n", kernelsz);
}

int main()
{
  FILE *bootblock = NULL, *kernel = NULL, *image = NULL;
  Elf32_Phdr *pphdr_bb = NULL, *pphdr_k = NULL;
  int kernelsz;

  if (!(bootblock = fopen("bootblock", "rb"))) {
    printf("failed to open bootblock\n");
    goto exit;
  }

  if (!(kernel = fopen("main", "rb"))) {
    printf("failed to open kernel\n");
    goto exit;
  }

  if (!(image = fopen("image", "wb"))) {
    printf("failed to open image\n");
    goto exit;
  } 

  pphdr_bb = read_exec_file(bootblock);
  pphdr_k = read_exec_file(kernel);

  if (!pphdr_bb || !pphdr_k) {
    printf("failed to read elf file\n");
    goto exit;
  }

  write_bootblock(image, bootblock, pphdr_bb);
  kernelsz = count_kernel_sectors(pphdr_k);
  record_kernel_sectors(image, kernelsz);
  write_kernel(image, kernel, pphdr_k, kernelsz);
  extend_opt(pphdr_bb, pphdr_k, kernelsz);

exit:
  free(pphdr_bb);
  free(pphdr_k);
  if (bootblock) fclose(bootblock);
  if (kernel) fclose(kernel);
  if (image) fclose(image);
  return 0;
}
