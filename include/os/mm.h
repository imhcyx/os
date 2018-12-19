#ifndef INCLUDE_MM_H_
#define INCLUDE_MM_H_

#include "type.h"

#define TLB_ENTRY_NUMBER 32

#define SWAP_FRAME_NUMBER_2 32
#define PAGE_FRAME_NUMBER_2 16

// VA: 0x00000000~0x7fffffff : 19 bits

#define SWAP_OFFSET_BASE 0x1000000
// PA
#define PAGE_FRAME_BASE 0x1800000
// VA
#define PTE_ALLOC_BASE 0xa0c00000

struct pte {
  uint32_t tlbindex;
  union {
    struct {
      unsigned global0:1;
      unsigned valid0:1;
      unsigned dirty0:1;
      unsigned cachable0:3;
      unsigned pfn0:24; // indicates location in swap when swapped out
      unsigned swapped:1; // masked out before writing to tlb
      unsigned allocated:1; // masked out before writing to tlb
    };
    uint32_t entrylo0;
  };
  union {
    struct {
      unsigned global1:1;
      unsigned valid1:1;
      unsigned dirty1:1;
      unsigned cachable1:3;
      unsigned pfn1:24;
      unsigned unused:2;
    };
    uint32_t entrylo1;
  };
};

struct pgd {
  struct pte *ptelist[512]; // 9 bits
};

// describes 2 consecutive page frames
struct pf2 {
  struct pte *pte;
};

void do_TLB_Refill();
void do_page_fault();

void init_page_table();
void init_TLB();
void init_swap();

extern void TLBexception_handler_entry(void);
extern void TLBexception_handler_begin(void);
extern void TLBexception_handler_end(void);

#ifdef ENABLE_SWAP
extern void sdread(void *buf, unsigned int base, int n);
extern void sdwrite(void *buf, unsigned int base, int n);
#endif

#endif
