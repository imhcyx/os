#include "mac.h"
#include "irq.h"
#include "type.h"
#include "screen.h"
#include "syscall.h"
#include "sched.h"
#include "test4.h"
#include "fs.h"

#define PA2VA(x) (0xa0000000|(x))

desc_t *send_desc;
desc_t *receive_desc;
uint32_t cnt = 1; //record the time of iqr_mac
//uint32_t buffer[PSIZE] = {0x00040045, 0x00000100, 0x5d911120, 0x0101a8c0, 0xfb0000e0, 0xe914e914, 0x00000801,0x45000400, 0x00010000, 0x2011915d, 0xc0a80101, 0xe00000fb, 0x14e914e9, 0x01080000};
uint32_t buffer[PSIZE] = {0xffffffff, 0x5500ffff, 0xf77db57d, 0x00450008, 0x0000d400, 0x11ff0040, 0xa8c073d8, 0x00e00101, 0xe914fb00, 0x0004e914, 0x0000, 0x005e0001, 0x2300fb00, 0x84b7f28b, 0x00450008, 0x0000d400, 0x11ff0040, 0xa8c073d8, 0x00e00101, 0xe914fb00, 0x0801e914, 0x0000};

static void mii_dul_force(mac_t *mac)
{
    reg_write_32(mac->dma_addr, 0x80); //?s
                                       //   reg_write_32(mac->dma_addr, 0x400);
    uint32_t conf = 0xc800;            //0x0080cc00;

    // loopback, 100M
    reg_write_32(mac->mac_addr, reg_read_32(mac->mac_addr) | (conf) | (1 << 8));
    //enable recieve all
    reg_write_32(mac->mac_addr + 0x4, reg_read_32(mac->mac_addr + 0x4) | 0x80000001);
}

static void start_tran(mac_t *mac)
{

   
}

void dma_control_init(mac_t *mac, uint32_t init_value)
{
    reg_write_32(mac->dma_addr + DmaControl, init_value);
    return;
}

void phy_regs_task1()
{

    mac_t test_mac;
    uint32_t i;
    uint32_t print_location = 2;

    test_mac.mac_addr = 0xbfe10000;
    test_mac.dma_addr = 0xbfe11000;

    test_mac.psize = PSIZE * 4; // 64bytes
    test_mac.pnum = PNUM;       // pnum

    send_desc_init(&test_mac);
    sys_move_cursor(1, print_location);
    printf("> [SEND TASK] filling buffers                   \n");
    for (i=0; i<PNUM; i++)
      memcpy((void*)test_mac.saddr+i*PSIZE*4, buffer, sizeof(buffer));

    dma_control_init(&test_mac, DmaStoreAndForward | DmaTxSecondFrame | DmaRxThreshCtrl128);
    clear_interrupt(&test_mac);

    mii_dul_force(&test_mac);

    //register_irq_handler(LS1C_MAC_IRQ, irq_mac);

    irq_enable(0);
    sys_move_cursor(1, print_location);
    printf("> [SEND TASK] start send package.               \n");

    uint32_t cnt = 0;
    i = 4;
    while (i > 0)
    {
        sys_net_send(test_mac.td, test_mac.td_phy);
        cnt += PNUM;
        sys_move_cursor(1, print_location);
        printf("> [SEND TASK] totally send package %d !        \n", cnt);
        i--;
    }
    sys_exit();
}

void dump_data_user(uint32_t *buf) {
  int i;
  screen_move_cursor(0, 3);
  for (i=0; i<16; i++) {
    printf("%08x %08x %08x %08x\n", buf[4*i], buf[4*i+1], buf[4*i+2], buf[4*i+3]);
  }
}

void phy_regs_task2()
{

    mac_t test_mac;
    uint32_t i;
    uint32_t ret;
    uint32_t print_location = 1;

    test_mac.mac_addr = 0xbfe10000;
    test_mac.dma_addr = 0xbfe11000;

    test_mac.psize = PSIZE*4;
    test_mac.pnum = 256;
    recv_desc_init(&test_mac);

    dma_control_init(&test_mac, DmaStoreAndForward | DmaTxSecondFrame | DmaRxThreshCtrl128);
    clear_interrupt(&test_mac);

    mii_dul_force(&test_mac);

    irq_enable(0);
    queue_init(&recv_block_queue);
    sys_move_cursor(1, print_location);
    printf("[RECV TASK] start recv:                    ");
    ret = sys_net_recv(test_mac.rd, test_mac.rd_phy, test_mac.daddr);
  
    /*ch_flag = 0;
    for (i = 0; i < PNUM; i++)
    {
        recv_flag[i] = 0;
    }*/

    uint32_t cnt = 0;
    uint32_t size, bufaddr;
    int sizeleft;
    desc_t *rdesc = (desc_t*)test_mac.rd;
    char recvbuf[256];
    int fd = sys_fopen("recvfile");
    while (1)
    {
        sys_move_cursor(1, print_location);
        printf("> [RECV TASK] waiting receive package %u.  \n", cnt);
        sys_wait_recv_package();
        while ((rdesc->tdes0 & DescOwnByDma) == 0) {
          bufaddr = PA2VA(rdesc->tdes2);
          memcpy(recvbuf, (char*)(bufaddr+54), 256);
          if (cnt == 0) {
            size = *(uint32_t*)recvbuf;
            sizeleft = size;
          }
          else {
            sys_fwrite(fd, recvbuf, sizeleft>256?256:sizeleft);
            sizeleft -= 256;
          }
          rdesc->tdes0 = 0x80000000;
          rdesc = (desc_t*)PA2VA(rdesc->tdes3);
          if (sizeleft>0) {
            cnt++;
            sys_move_cursor(1, print_location);
            printf("> [RECV TASK] processed package %u/%u.   \n", cnt-1, size/256);
          }
        }
        if (cnt>=size/256) break;
    }
    sys_close(fd);
    sys_move_cursor(1, print_location);
    printf("> [RECV TASK] finished.                    ", cnt);

    sys_exit();
}

void phy_regs_task3()
{
    uint32_t print_location = 1;
    sys_move_cursor(1, print_location);
    printf("> [INIT] Waiting for MAC initialization .\n");
    sys_init_mac();
    sys_move_cursor(1, print_location);
    printf("> [INIT] MAC initialization succeeded.           \n");
    sys_exit();
}
