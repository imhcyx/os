#include "mm.h"

void init_page_table() {
}

static inline void fill_tlb_simple
(
 uint32_t vpn2,
 uint32_t pfn0,
 uint32_t pfn1,
 uint32_t index
)
{
  __asm__ volatile (
    "mtc0 $zero, $5\n" // set PageMask
    "mtc0 %0, $10\n"  // set EntryHi
    "mtc0 %1, $2\n"   // set EntryLo0
    "mtc0 %2, $3\n"   // set EntryLo1
    "mtc0 %3, $0\n"   // set Index
    "tlbwi\n"
    ::
    "r" (vpn2 << 13),
    "r" (pfn0 << 6 | 0x17), // C=2, D=1, V=1, G=1
    "r" (pfn1 << 6 | 0x17), // C=2, D=1, V=1, G=1
    "r" (index)
  );
}

void tlbr
(
 uint32_t *pagemask,
 uint32_t *entryhi,
 uint32_t *entrylo0,
 uint32_t *entrylo1,
 uint32_t index
)
{
  __asm__ volatile (
    "mtc0 %4, $0\n"
    "tlbr\n"
    "mfc0 $t0, $5\n"
    "sw $t0, %0\n"
    "mfc0 $t0, $10\n"
    "sw $t0, %1\n"
    "mfc0 $t0, $2\n"
    "sw $t0, %2\n"
    "mfc0 $t0, $3\n"
    "sw $t0, %3\n"
    :
    "=m" (*pagemask),
    "=m" (*entryhi),
    "=m" (*entrylo0),
    "=m" (*entrylo1)
    : "r" (index)
    : "t0"
  );
}

void init_TLB() {
  // map VA 0x00000000~0x00040000 to PA 0x1000000~0x1040000
  uint32_t index;
  for (index = 0; index < 32; index++)
    fill_tlb_simple(index, 0x1000 + index*2, 0x1001 + index*2, index);
}

void init_swap() {
}
