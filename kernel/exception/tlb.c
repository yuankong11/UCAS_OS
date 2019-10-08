#include "tlb.h"
#include "type.h"
#include "sched.h"
#include "debug.h"
#include "screen.h"

void TLB_exception_helper(int is_TLB_refill)
{
    int pool_num;
    uint32_t PFN;
    uint32_t badvaddr = current_running->user_regs_context.cp0_badvaddr;

    uint32_t EntryHi;
    asm(
        "mfc0 %0, $10 \n" // EntryHi
        : "=&r"(EntryHi)
    );

    /* ----------- BADVADDR ----------- *
    * NAME | 0 | PDN | PTN | 0 | OFFSET |
    * -----------------------------------
    * BITS | 1 |  9  |  9  | 1 |   12   |
    * ---------------------------------*/

    uint32_t page_dir_num = badvaddr >> 22;
    uint32_t page_table_num = (badvaddr >> 13) & 0x1ff;
    if(!current_running->page_dir[page_dir_num].valid)
    {
        pool_num = pt_pool_alloc();
        if(pool_num == -1)
            panic("Empty page table pool.");
        current_running->page_dir[page_dir_num].pool_num = pool_num;
        page_table_init(page_table_pool[pool_num]);
        current_running->page_dir[page_dir_num].valid = 1;
    }
    else
    {
        pool_num = current_running->page_dir[page_dir_num].pool_num;
    }

    if(!page_table_pool[pool_num][page_table_num].valid)
    {
        int pm_index = pm_alloc();
        if(pm_index == -1)
        {
            swap_out();
            pm_index = pm_alloc();
        }
        PFN = ((uint32_t)(PM_BASE + pm_index*PM_SCHE_SIZE)) >> 12;
        pm_pid[pm_index] = current_running->pid;
        pm_vpn[pm_index] = badvaddr & 0xfffff000;
        page_table_pool[pool_num][page_table_num].addr.PFN = PFN;
        page_table_pool[pool_num][page_table_num].valid = 1;
        page_table_pool[pool_num][page_table_num].present = 1;
    }
    else
    {
        if(!page_table_pool[pool_num][page_table_num].present)
            swap_in(&page_table_pool[pool_num][page_table_num], badvaddr);
        PFN = page_table_pool[pool_num][page_table_num].addr.PFN;
    }
    uint32_t EntryLo0 = (PFN << 6) | 0b010110;
    uint32_t EntryLo1 = ((PFN+1) << 6) | 0b010110;

    if(is_TLB_refill)
    {
        asm(
            "mtc0 %0, $2  \n" // EntryLo0
            "mtc0 %1, $3  \n" // EntryLo1
            "mtc0 %2, $10 \n" // EntryHi
            "tlbwr        \n"
            :
            : "r"(EntryLo0), "r"(EntryLo1), "r"(EntryHi)
        );
    }
    else
    {
        asm(
            "mtc0 %0, $2  \n" // EntryLo0
            "mtc0 %1, $3  \n" // EntryLo1
            "mtc0 %2, $10 \n" // EntryHi
            "tlbp         \n" // Index
            "tlbwi        \n"
            :
            : "r"(EntryLo0), "r"(EntryLo1), "r"(EntryHi)
        );
    }
}