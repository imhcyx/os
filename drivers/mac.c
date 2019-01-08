#include "mac.h"
#include "irq.h"

queue_t recv_block_queue;

#define PA2VA(x) (0xa0000000|(x))

uint32_t reg_read_32(uint32_t addr)
{
    return *((uint32_t *)addr);
}
uint32_t read_register(uint32_t base, uint32_t offset)
{
    uint32_t addr = base + offset;
    uint32_t data;

    data = *(volatile uint32_t *)addr;
    return data;
}

void reg_write_32(uint32_t addr, uint32_t data)
{
    *((uint32_t *)addr) = data;
}

static void gmac_get_mac_addr(uint8_t *mac_addr)
{
    uint32_t addr;

    addr = read_register(GMAC_BASE_ADDR, GmacAddr0Low);
    mac_addr[0] = (addr >> 0) & 0x000000FF;
    mac_addr[1] = (addr >> 8) & 0x000000FF;
    mac_addr[2] = (addr >> 16) & 0x000000FF;
    mac_addr[3] = (addr >> 24) & 0x000000FF;

    addr = read_register(GMAC_BASE_ADDR, GmacAddr0High);
    mac_addr[4] = (addr >> 0) & 0x000000FF;
    mac_addr[5] = (addr >> 8) & 0x000000FF;
}

/* print DMA regs */
void print_dma_regs()
{
    uint32_t regs_val1, regs_val2;

    printk(">>[DMA Register]\n");

    // [0] Bus Mode Register
    regs_val1 = read_register(DMA_BASE_ADDR, DmaBusMode);

    printk("  [0] Bus Mode : 0x%x, ", regs_val1);

    // [3-4] RX/TX List Address Register
    regs_val1 = read_register(DMA_BASE_ADDR, DmaRxBaseAddr);
    regs_val2 = read_register(DMA_BASE_ADDR, DmaTxBaseAddr);
    printk("  [3-4] TX/RX : 0x%x/0x%x\n", regs_val2, regs_val1);

    // [5] Status Register
    regs_val1 = read_register(DMA_BASE_ADDR, DmaStatus);
    printk("  [5] Status : 0x%x, ", regs_val1);

    // [6] Operation Mode Register
    regs_val1 = read_register(DMA_BASE_ADDR, DmaControl);
    printk("  [6] Control : 0x%x\n", regs_val1);

    // [7] Interrupt Enable Register
    regs_val1 = read_register(DMA_BASE_ADDR, DmaInterrupt);
    printk("  [7] Interrupt : 0x%x, ", regs_val1);

    // [8] Miss
    regs_val1 = read_register(DMA_BASE_ADDR, DmaMissedFr);
    printk("  [8] Missed : 0x%x\n", regs_val1);

    // [18-19] Current Host TX/RX Description Register
    regs_val1 = read_register(DMA_BASE_ADDR, DmaTxCurrDesc);
    regs_val2 = read_register(DMA_BASE_ADDR, DmaRxCurrDesc);
    printk("  [18-19] Current Host TX/RX Description : 0x%x/0x%x\n", regs_val1, regs_val2);

    // [20-21] Current Host TX/RX Description Register
    regs_val1 = read_register(DMA_BASE_ADDR, DmaTxCurrAddr);
    regs_val2 = read_register(DMA_BASE_ADDR, DmaRxCurrAddr);
    printk("  [20-21] Current Host TX/RX Buffer Address : 0x%x/0x%x\n", regs_val1, regs_val2);
}

/* print DMA regs */
void print_mac_regs()
{
    printk(">>[MAC Register]\n");
    uint32_t regs_val1, regs_val2;
    uint8_t mac_addr[6];

    // [0] MAC Configure Register
    regs_val1 = read_register(GMAC_BASE_ADDR, GmacConfig);
    printk("  [0] Configure : 0x%x, ", regs_val1);

    // [1] MAC Frame Filter
    regs_val1 = read_register(GMAC_BASE_ADDR, GmacFrameFilter);
    printk("  [1] Frame Filter : 0x%x\n", regs_val1);

    // [2-3] Hash Table High/Low Register
    regs_val1 = read_register(GMAC_BASE_ADDR, GmacHashHigh);
    regs_val2 = read_register(GMAC_BASE_ADDR, GmacHashLow);
    printk("  [2-3] Hash Table High/Low : 0x%x-0x%x\n", regs_val1, regs_val2);

    // [6] Flow Control Register
    regs_val1 = read_register(GMAC_BASE_ADDR, GmacFlowControl);
    printk("  [6] Flow Control : 0x%x, ", regs_val1);

    // [8] Version Register
    regs_val1 = read_register(GMAC_BASE_ADDR, GmacVersion);
    printk("  [8] Version : 0x%x\n", regs_val1);

    // [14] Interrupt Status Register and Interrupt Mask
    regs_val1 = read_register(GMAC_BASE_ADDR, GmacInterruptStatus);
    regs_val2 = read_register(GMAC_BASE_ADDR, GmacInterruptMask);
    printk("  [14-15] Interrupt Status/Mask : 0x%x/0x%x\n", regs_val1, regs_val2);

    // MAC address
    gmac_get_mac_addr(mac_addr);
    printk("  [16-17] Mac Addr : %X:%X:%X:%X:%X:%X\n", mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
}

void printf_dma_regs()
{
    uint32_t regs_val1, regs_val2;

    printf(">>[DMA Register]\n");

    // [0] Bus Mode Register
    regs_val1 = read_register(DMA_BASE_ADDR, DmaBusMode);

    printf("  [0] Bus Mode : 0x%x, ", regs_val1);

    // [3-4] RX/TX List Address Register
    regs_val1 = read_register(DMA_BASE_ADDR, DmaRxBaseAddr);
    regs_val2 = read_register(DMA_BASE_ADDR, DmaTxBaseAddr);
    printf("  [3-4] RX/TX : 0x%x/0x%x\n", regs_val1, regs_val2);

    // [5] Status Register
    regs_val1 = read_register(DMA_BASE_ADDR, DmaStatus);
    printf("  [5] Status : 0x%x, ", regs_val1);

    // [6] Operation Mode Register
    regs_val1 = read_register(DMA_BASE_ADDR, DmaControl);
    printf("  [6] Control : 0x%x\n", regs_val1);

    // [7] Interrupt Enable Register
    regs_val1 = read_register(DMA_BASE_ADDR, DmaInterrupt);
    printf("  [7] Interrupt : 0x%x, ", regs_val1);

    // [8] Miss
    regs_val1 = read_register(DMA_BASE_ADDR, DmaMissedFr);
    printf("  [8] Missed : 0x%x\n", regs_val1);

    // [18-19] Current Host TX/RX Description Register
    regs_val1 = read_register(DMA_BASE_ADDR, DmaTxCurrDesc);
    regs_val2 = read_register(DMA_BASE_ADDR, DmaRxCurrDesc);
    printf("  [18-19] Current Host TX/RX Description : 0x%x/0x%x\n", regs_val1, regs_val2);

    // [20-21] Current Host TX/RX Description Register
    regs_val1 = read_register(DMA_BASE_ADDR, DmaTxCurrAddr);
    regs_val2 = read_register(DMA_BASE_ADDR, DmaRxCurrAddr);
    printf("  [20-21] Current Host TX/RX Buffer Address : 0x%x/0x%x\n", regs_val1, regs_val2);
}

/* print DMA regs */
void printf_mac_regs()
{
    printf(">>[MAC Register]\n");
    uint32_t regs_val1, regs_val2;
    uint8_t mac_addr[6];

    // [0] MAC Configure Register
    regs_val1 = read_register(GMAC_BASE_ADDR, GmacConfig);
    printf("  [0] Configure : 0x%x, ", regs_val1);

    // [1] MAC Frame Filter
    regs_val1 = read_register(GMAC_BASE_ADDR, GmacFrameFilter);
    printf("  [1] Frame Filter : 0x%x\n", regs_val1);

    // [2-3] Hash Table High/Low Register
    regs_val1 = read_register(GMAC_BASE_ADDR, GmacHashHigh);
    regs_val2 = read_register(GMAC_BASE_ADDR, GmacHashLow);
    printf("  [2-3] Hash Table High/Low : 0x%x-0x%x\n", regs_val1, regs_val2);

    // [6] Flow Control Register
    regs_val1 = read_register(GMAC_BASE_ADDR, GmacFlowControl);
    printf("  [6] Flow Control : 0x%x, ", regs_val1);

    // [8] Version Register
    regs_val1 = read_register(GMAC_BASE_ADDR, GmacVersion);
    printf("  [8] Version : 0x%x\n", regs_val1);

    // [14] Interrupt Status Register and Interrupt Mask
    regs_val1 = read_register(GMAC_BASE_ADDR, GmacInterruptStatus);
    regs_val2 = read_register(GMAC_BASE_ADDR, GmacInterruptMask);
    printf("  [14-15] Interrupt Status/Mask : 0x%x/0x%x\n", regs_val1, regs_val2);

    // MAC address
    gmac_get_mac_addr(mac_addr);
    printf("  [16-17] Mac Addr : %X:%X:%X:%X:%X:%X\n", mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
}

void print_tx_dscrb(mac_t *mac)
{
    uint32_t i;
    printf("send buffer mac->saddr=0x%x ", mac->saddr);
    printf("mac->saddr_phy=0x%x ", mac->saddr_phy);
    printf("send discrb mac->td_phy=0x%x\n", mac->td_phy);
#if 0
    desc_t *send=mac->td;
    for(i=0;i<mac->pnum;i++)
    {
        printf("send[%d].tdes0=0x%x ",i,send[i].tdes0);
        printf("send[%d].tdes1=0x%x ",i,send[i].tdes1);
        printf("send[%d].tdes2=0x%x ",i,send[i].tdes2);
        printf("send[%d].tdes3=0x%x ",i,send[i].tdes3);
    }
#endif
}

void print_rx_dscrb(mac_t *mac)
{
    uint32_t i;
    printf("recieve buffer add mac->daddr=0x%x ", mac->daddr);
    printf("mac->daddr_phy=0x%x ", mac->daddr_phy);
    printf("recieve discrb add mac->rd_phy=0x%x\n", mac->rd_phy);
    desc_t *recieve = (desc_t *)mac->rd;
#if 0
    for(i=0;i<mac->pnum;i++)
    {
        printf("recieve[%d].tdes0=0x%x ",i,recieve[i].tdes0);
        printf("recieve[%d].tdes1=0x%x ",i,recieve[i].tdes1);
        printf("recieve[%d].tdes2=0x%x ",i,recieve[i].tdes2);
        printf("recieve[%d].tdes3=0x%x\n",i,recieve[i].tdes3);
    }
#endif
}

/**
 * Clears all the pending interrupts.
 * If the Dma status register is read then all the interrupts gets cleared
 * @param[in] pointer to synopGMACdevice.
 * \return returns void.
 */
void clear_interrupt()
{
    uint32_t data;
    data = reg_read_32(0xbfe11000 + DmaStatus);
    reg_write_32(0xbfe11000 + DmaStatus, data);
}

#define INT1_EN  0xbfd0105c
#define INT1_CLR 0xbfd01064
#define INT1_POL 0xbfd01068
#define INT1_EDGE 0xbfd0106c

desc_t *rlast = NULL;
#if 0
void irq_mac(void)
{
  if (rlast && (rlast->tdes0 & DescOwnByDma) == 0) {
    do_unblock_one(&recv_block_queue);
    rlast = NULL;
  }
  clear_interrupt();
  reg_write_32(INT1_CLR, 0xffffffff);
}
#endif

void irq_mac() {
  clear_interrupt();
  do_unblock_one(&recv_block_queue);
  reg_write_32(INT1_CLR, 0xffffffff);
}

uint32_t bonus_count = 0;
void irq_mac_bonus() {
  static desc_t *rdesc = NULL;
  clear_interrupt();
  if (!rdesc)
    rdesc = (desc_t*)PA2VA(reg_read_32(DMA_BASE_ADDR + DmaRxBaseAddr));
  while ((rdesc->tdes0 & DescOwnByDma) == 0) {
    bonus_count++;
    rdesc->tdes0 = 0x80000000;
    rdesc = (desc_t*)PA2VA(rdesc->tdes3);
  }
  reg_write_32(INT1_CLR, 0xffffffff);
}

void irq_enable(int IRQn)
{
  reg_write_32(INT1_CLR, 0xffffffff);
  reg_write_32(INT1_POL, 0xffffffff);
  reg_write_32(INT1_EDGE, 0);
  reg_write_32(INT1_EN, reg_read_32(INT1_EN)|(1<<3));
}

void dump_data(uint32_t *buf) {
  int i;
  vt100_move_cursor(1, 5);
  for (i=0; i<16; i++) {
    printk("%08x %08x %08x %08x\n", buf[4*i], buf[4*i+1], buf[4*i+2], buf[4*i+3]);
  }
}

// parameter unused
void check_recv(mac_t *test_mac)
{
    desc_t *rdesc = (desc_t*)test_mac->rd;
    while ((rdesc->tdes0 & DescOwnByDma) == 0) {
      dump_data((uint32_t*)PA2VA(rdesc->tdes2));
      if ((rdesc->tdes1 & RxDescEndOfRing) != 0) {
        rdesc = NULL;
        return;
      }
      rdesc = (desc_t*)PA2VA(rdesc->tdes3);
    }
}

void set_sram_ctr()
{
    *((volatile unsigned int *)0xbfd00420) = 0x8000; /* 使能GMAC0 */
}
static void s_reset(mac_t *mac) //reset mac regs
{
    uint32_t time = 1000000;
    reg_write_32(mac->dma_addr, 0x01);

    while ((reg_read_32(mac->dma_addr) & 0x01))
    {
        reg_write_32(mac->dma_addr, 0x01);
        while (time)
        {
            time--;
        }
    };
}
void disable_interrupt_all(mac_t *mac)
{
    reg_write_32(mac->dma_addr + DmaInterrupt, DmaIntDisable);
    return;
}
void set_mac_addr(mac_t *mac)
{
    uint32_t data;
    uint8_t MacAddr[6] = {0x00, 0x55, 0x7b, 0xb5, 0x7d, 0xf7};
    uint32_t MacHigh = 0x40, MacLow = 0x44;
    data = (MacAddr[5] << 8) | MacAddr[4];
    reg_write_32(mac->mac_addr + MacHigh, data);
    data = (MacAddr[3] << 24) | (MacAddr[2] << 16) | (MacAddr[1] << 8) | MacAddr[0];
    reg_write_32(mac->mac_addr + MacLow, data);
}

uint32_t do_net_recv(uint32_t rd, uint32_t rd_phy, uint32_t daddr)
{
    reg_write_32(DMA_BASE_ADDR + DmaRxBaseAddr, rd_phy);
    //PLEASE enable MAC-RX
    reg_write_32(GMAC_BASE_ADDR + GmacConfig, reg_read_32(GMAC_BASE_ADDR + GmacConfig) | GmacRxEnable);

    reg_write_32(DMA_BASE_ADDR + 0x18, reg_read_32(DMA_BASE_ADDR + 0x18) | 0x02200002); // start tx, rx
    reg_write_32(DMA_BASE_ADDR + 0x1c, 0x10001 | (1 << 6));
    
    //you should add some code to start recv and check recv packages
    desc_t *p = (desc_t*)rd;
    uint32_t num = 0;
    int i;
    while (1) {
      num++;
      p->tdes0 |= DescOwnByDma;
      if ((p->tdes1 & RxDescEndOfRing) != 0) break;
      p = (desc_t*)PA2VA(p->tdes3);
    }
    for (i=0; i<num; i++)
      reg_write_32(DMA_BASE_ADDR + DmaRxPollDemand, 1);
    rlast = p;
    return 0;
}

void do_net_send(uint32_t td, uint32_t td_phy)
{
    reg_write_32(DMA_BASE_ADDR + DmaTxBaseAddr, td_phy);
    //PLEASE enable MAC-TX
    reg_write_32(GMAC_BASE_ADDR + GmacConfig, reg_read_32(GMAC_BASE_ADDR + GmacConfig) | GmacTxEnable);
    
    reg_write_32(DMA_BASE_ADDR + 0x18, reg_read_32(DMA_BASE_ADDR + 0x18) | 0x02202000); //0x02202002); // start tx, rx
    reg_write_32(DMA_BASE_ADDR + 0x1c, 0x10001 | (1 << 6));

    //you should add some code to start send packages
    desc_t *p = (desc_t*)td;
    uint32_t num = 0;
    int i;
    while (1) {
      num++;
      p->tdes0 |= DescOwnByDma;
      if ((p->tdes1 & TxDescEndOfRing) != 0) break;
      p = (desc_t*)PA2VA(p->tdes3);
    }
    for (i=0; i<num; i++)
      reg_write_32(DMA_BASE_ADDR + DmaTxPollDemand, 1);
}

void do_init_mac(void)
{
    mac_t test_mac;
    uint32_t i;

    test_mac.mac_addr = 0xbfe10000;
    test_mac.dma_addr = 0xbfe11000;

    test_mac.psize = PSIZE * 4; // 64bytes
    test_mac.pnum = PNUM;       // pnum

    set_sram_ctr(); /* 使能GMAC0 */
    s_reset(&test_mac);
    disable_interrupt_all(&test_mac);
    set_mac_addr(&test_mac);
}

void do_wait_recv_package(void)
{
    do_block(&recv_block_queue);
}

uint32_t desc_allocaddr = NET_DESC_ALLOCBASE_PHY;
uint32_t buf_allocaddr = NET_BUF_ALLOCBASE_PHY;

void send_desc_init(mac_t *mac) {
  uint32_t desc_phy, buf_phy;
  desc_t *desc, *p;
  int i;

  desc_phy = desc_allocaddr;
  desc_allocaddr += sizeof(desc_t) * mac->pnum;
  buf_phy = buf_allocaddr;
  buf_allocaddr += mac->psize * mac->pnum;

  desc = (desc_t*)PA2VA(desc_phy);
  p = desc;
  
  for (i=0; i<mac->pnum; i++) {
    p->tdes0 = 0;
    p->tdes1 = ((i==mac->pnum-1) ? 0x63000000 : 0x61000000) | mac->psize;
    p->tdes2 = buf_phy + mac->psize*i;
    p->tdes3 = (i==mac->pnum-1) ? desc_phy : desc_phy+sizeof(desc_t)*(i+1);
    p++;
  }

  mac->saddr = PA2VA(buf_phy);
  mac->saddr_phy = buf_phy;
  mac->td = PA2VA(desc_phy);
  mac->td_phy = desc_phy;
}

void recv_desc_init(mac_t *mac) {
  uint32_t desc_phy, buf_phy;
  desc_t *desc, *p;
  int i;

  desc_phy = desc_allocaddr;
  desc_allocaddr += sizeof(desc_t) * mac->pnum;
  buf_phy = buf_allocaddr;
  buf_allocaddr += mac->psize * mac->pnum;

  desc = (desc_t*)PA2VA(desc_phy);
  p = desc;
  
  for (i=0; i<mac->pnum; i++) {
    p->tdes0 = 0;
    p->tdes1 = ((i==mac->pnum-1) ? 0x03000000 : 0x01000000) | mac->psize;
    p->tdes2 = buf_phy + mac->psize*i;
    p->tdes3 = (i==mac->pnum-1) ? desc_phy : desc_phy+sizeof(desc_t)*(i+1);
    p++;
  }

  mac->daddr = PA2VA(buf_phy);
  mac->daddr_phy = buf_phy;
  mac->rd = PA2VA(desc_phy);
  mac->rd_phy = desc_phy;
}
