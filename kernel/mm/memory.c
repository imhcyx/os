#include "sched.h"
#include "mm.h"

int rand();

struct pf2 pageframe[PAGE_FRAME_NUMBER_2];
#ifdef ENABLE_SWAP
struct pf2 swapframe[SWAP_FRAME_NUMBER_2];
#endif

struct pte *pte_alloc_ptr = (struct pte*)PTE_ALLOC_BASE;

// convert pfn to unmapped va
#define PFN2VA(x) ((x)<<12 | 0xa0000000)

struct pte *alloc_ptelist() {
  struct pte *ret = pte_alloc_ptr;
  pte_alloc_ptr += 512;
  memset(ret, 0, sizeof(struct pte)*512);
  return ret;
}

static inline void fill_tlb
(
  uint32_t vpn2,
  uint32_t asid,
  struct pte* pte,
  uint32_t index
) {
  __asm__ volatile (
    "mtc0 $zero, $5\n" // set PageMask
    "mtc0 %0, $10\n"  // set EntryHi
    "mtc0 %1, $2\n"   // set EntryLo0
    "mtc0 %2, $3\n"   // set EntryLo1
    "mtc0 %3, $0\n"   // set Index
    "tlbwi\n"
    ::
    "r" (vpn2 << 13 | asid),
    "r" (pte->entrylo0 & 0x3fffffff),
    "r" (pte->entrylo1 & 0x3fffffff),
    "r" (index)
  );
  pte->tlbindex = index;
}

static inline void update_tlb
(
  struct pte* pte
) {
  __asm__ volatile (
    "mtc0 %0, $0\n"   // set Index
    "tlbr\n"
    "mtc0 %1, $2\n"   // set EntryLo0
    "mtc0 %2, $3\n"   // set EntryLo1
    "tlbwi\n"
    ::
    "r" (pte->tlbindex),
    "r" (pte->entrylo0 & 0x3fffffff),
    "r" (pte->entrylo1 & 0x3fffffff)
  );
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

#ifdef ENABLE_SWAP
void store_pageframe(int pfindex) {
  struct pte *pte = pageframe[pfindex].pte;
  int found = 0;
  int i;
  for (i=0; i<SWAP_FRAME_NUMBER_2; i++) {
    if (swapframe[i].pte == NULL) {
      found = 1;
      break;
    }
  }
  if (!found)
    panic("out of swap");
  swapframe[i].pte = pte;
  pageframe[pfindex].pte = NULL;
  pte->valid0 = 0;
  pte->valid1 = 0;
  pte->swapped = 1;
  sdwrite((void*)PFN2VA(pte->pfn0), SWAP_OFFSET_BASE+4096*i*2, 4096);
  sdwrite((void*)PFN2VA(pte->pfn1), SWAP_OFFSET_BASE+4096*(i*2+1), 4096);
  pte->pfn0 = i;
  update_tlb(pte);
}
#endif

int alloc_pageframe(struct pte *pte) {
  int i;
  int found = 0;
  for (i=0; i<PAGE_FRAME_NUMBER_2; i++) {
    if (pageframe[i].pte == NULL) {
      found = 1;
      break;
    }
  }
  if (!found) {
#ifdef ENABLE_SWAP
    i = rand() % PAGE_FRAME_NUMBER_2;
    store_pageframe(i);
#else
    panic("out of memory");
#endif
  }
  pageframe[i].pte = pte;
  return i;
}

#ifdef ENABLE_SWAP
void load_pageframe(int sfindex) {
  struct pte *pte = swapframe[sfindex].pte;
  int i = alloc_pageframe(pte);
  pageframe[i].pte = pte;
  swapframe[sfindex].pte = NULL;
  pte->pfn0 = 0x1000 + i*2;
  pte->pfn1 = 0x1001 + i*2;
  sdread((void*)PFN2VA(pte->pfn0), SWAP_OFFSET_BASE+4096*sfindex*2, 4096);
  sdread((void*)PFN2VA(pte->pfn1), SWAP_OFFSET_BASE+4096*(sfindex*2+1), 4096);
  pte->swapped = 0;
  pte->valid0 = 1;
  pte->valid1 = 1;
}
#endif

void validate_pte(struct pte *pte) {
  int pfindex;
  if (!pte->allocated) {
    pfindex = alloc_pageframe(pte);
    pte->allocated = 1;
    pte->pfn0 = 0x1000 + pfindex*2;
    pte->pfn1 = 0x1001 + pfindex*2;
  }
#ifdef ENABLE_SWAP
  if (pte->swapped) {
    load_pageframe(pte->pfn0);
  }
#endif
  pte->cachable0 = 2;
  pte->cachable1 = 2;
  pte->dirty0 = 1;
  pte->dirty1 = 1;
  pte->valid0 = 1;
  pte->valid1 = 1;
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

void do_TLB_Refill() {
  static uint32_t index;
  uint32_t context, badvpn2;
  uint32_t pgdindex, pteindex;
  struct pte *pte;
  __asm__ volatile ("mfc0 %0, $4\n" : "=r" (context));
  badvpn2 = (context >> 4) & 0x7ffff;
  pgdindex = badvpn2 >> 9;
  pteindex = badvpn2 & 0x1ff;
  if (current_running->pgd.ptelist[pgdindex] == NULL) {
    current_running->pgd.ptelist[pgdindex] = alloc_ptelist();
  }
  pte = current_running->pgd.ptelist[pgdindex] + pteindex;
  fill_tlb(badvpn2, current_running->pid, pte, index);
  index = (index+1) & 0xff;
}

void do_page_fault() {
  uint32_t index;
  uint32_t vpn2 = current_running->context.cp0_badvaddr >> 13;
  uint32_t pgdindex, pteindex;
  struct pte *pte;
  __asm__ volatile (
    "mtc0 %1, $10\n"
    "tlbp\n"
    "mfc0 %0, $0\n"
    : "=r" (index)
    : "r" (vpn2 << 13 | current_running->pid)
  );
  pgdindex = vpn2 >> 9;
  pteindex = vpn2 & 0x1ff;
  pte = current_running->pgd.ptelist[pgdindex] + pteindex;
  validate_pte(pte);
  fill_tlb(vpn2, current_running->pid, pte, index);
}

void init_page_table() {
  char *src = (char*)TLBexception_handler_begin;
  char *dst = (char*)0x80000000;
  while (src < (char*)TLBexception_handler_end)
    *dst++ = *src++;
  memset(pageframe, 0, sizeof(pageframe));
}

void init_TLB() {
  // map VA 0x00000000~0x00040000 to PA 0x1000000~0x1040000
  /*
  uint32_t index;
  for (index = 0; index < 32; index++)
    fill_tlb_simple(index, 0x1000 + index*2, 0x1001 + index*2, index);
  */
  int i;
  for (i=0;i<32;i++) {
    __asm__ volatile (
      "mtc0 $zero, $5\n" // set PageMask
      "mtc0 $zero, $10\n"  // set EntryHi
      "mtc0 $zero, $2\n"   // set EntryLo0
      "mtc0 $zero, $3\n"   // set EntryLo1
      "mtc0 %0, $0\n"   // set Index
      "tlbwi\n"
      ::
      "r" (i)
    );
  }
}

void init_swap() {
}
