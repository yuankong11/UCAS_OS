#include "test_bonus.h"
#include "type.h"
#include "mac.h"
#include "syscall.h"
#include "fs.h"

#define ELF_FLAG 0x464c457f

void test_bonus1()
{
    // recv
    mac_t test_mac;
    int i;
    int print_location = 0;

    sys_init_mac();

    test_mac.psize = 2047;
    test_mac.pnum = 64;
    recv_desc_init(&test_mac);

    dma_control_init(DmaStoreAndForward | DmaTxSecondFrame | DmaRxThreshCtrl128);
    clear_dma_interrupt();
    mii_dul_force();

    sys_move_cursor(0, print_location);
    printf("> [RECV TASK] waiting receive package.                             ");
    sys_net_recv();

    for(i = 0; i < 64; ++i)
    {
        if(rdesc[i].des0 >> 31)
            sys_net_wait_recv();
        if(rbuffer[i][60] == 127 && rbuffer[i][61] == 69 && rbuffer[i][62] == 76 && rbuffer[i][63] == 70)
        {
            sys_move_cursor(0, print_location);
            printf("> [RECV TASK] receive executable file, saved at 1.exe.     ");
            break;
        }
    }

    if((rdesc[i].des0 >> 15) & 0x1)
    {
        sys_move_cursor(0, print_location);
        printf("Error: recv error.                                                  ");
        sys_exit();
    }
    sys_move_cursor(0, print_location);
    printf("> [RECV TASK] writing...                                            ");
    int size = (rdesc[i].des0 >> 16) & 0x3fff;
    size -= 60;
    int fd = sys_fopen("1.exe", O_WR);
    if(fd == -1)
    {
        sys_move_cursor(0, print_location);
        printf("Error: open file failed.                                            ");
        sys_exit();
    }
    int ret = sys_fwrite(fd, (uint8_t *)&rbuffer[i][60], size);
    sys_move_cursor(0, print_location);
    printf("> [RECV TASK] writing file finished, size: %d                    ", ret);
    sys_fclose(fd);

    sys_exit();
}
