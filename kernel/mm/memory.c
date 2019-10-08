#include "memory.h"
#include "slist.h"
#include "sched.h"
#include "software.h"
#include "sem.h"
#include "time.h"

page_table_pool_t page_table_pool;
node_t pt_pool_slist[PAGE_TABLE_POOL_SLIST_SIZE];

node_t pm_slist[PM_SLIST_SIZE];
uint32_t pm_pid[PM_SLIST_SIZE];
uint32_t pm_vpn[PM_SLIST_SIZE];

node_t swap_area_slist[SWAP_AREA_SIZE];

void page_dir_init(page_dir_t pd)
{
    int i;
    for(i = 0; i < PDE_NUMBER; ++i)
        pd[i].valid = 0;
}

void page_table_init(page_table_t pt)
{
    int i;
    for(i = 0; i < PTE_NUMBER; ++i)
        pt[i].valid = 0;
}

int pm_alloc()
{
    return slist_alloc(pm_slist) - 1;
}

int pm_free(int index)
{
    return slist_free(pm_slist, index + 1, PM_SLIST_SIZE);
}

int pt_pool_alloc()
{
    return slist_alloc(pt_pool_slist) - 1;
}

int pt_pool_free(int index)
{
    return slist_free(pt_pool_slist, index + 1, PAGE_TABLE_POOL_SLIST_SIZE);
}

uint32_t swap_area_alloc()
{
    return SWAP_AREA_BASE + (slist_alloc(swap_area_slist) - 1)*SWAP_AREA_UINT_SIZE;
}

int swap_area_free(uint32_t address)
{
    int index = (address - SWAP_AREA_BASE)/SWAP_AREA_UINT_SIZE + 1;
    return slist_free(swap_area_slist, index, SWAP_AREA_SIZE);
}

static int swap_scheduler()
{
    return 2 + get_timer() % (PM_SLIST_SIZE-3); // randomly
}

void swap_out()
{
    //do_semaphore_down(&free_io_tasks);

    int pm_index = swap_scheduler();
    uint8_t  pid = pm_pid[pm_index];
    uint32_t vpn = pm_vpn[pm_index];
    uint32_t page_dir_num = vpn >> 22;
    uint32_t page_table_num = (vpn >> 13) & 0x1ff;
    int pool_num = pcb[pid].page_dir[page_dir_num].pool_num;

    pm_free(pm_index); // todo: should alloc again, otherwise other process will apply for it

    uint32_t PA = PM_BASE + pm_index*PM_SCHE_SIZE;
    unsigned char *buffer = (unsigned char *)(PA | 0xa0000000); // use uncached & unmapped segment
    uint32_t base = swap_area_alloc();

    page_table_pool[pool_num][page_table_num].present = 0;
    page_table_pool[pool_num][page_table_num].addr.swap_area_addr = base;

    uint32_t EntryHi = (vpn & 0xffffe000) | pid;
    set_TLB_invalid(EntryHi);

    if(current_running != &pcb[pid])
        queue_remove(pcb[pid].queue, &pcb[pid]); // to assure this task won't be scheduled
                                                 // otherwise the memory will be inconsistent

    //sdwrite(buffer, base, SWAP_AREA_UINT_SIZE);

    io_task.type = IO_TASK_WRITE;
    io_task.buffer = buffer;
    io_task.base = base;
    io_task.n = SWAP_AREA_UINT_SIZE;
    do_semaphore_up(&io_tasks);
    do_semaphore_down(&io_done);

    if(current_running != &pcb[pid])
        queue_push(pcb[pid].queue, &pcb[pid]);

    //do_semaphore_up(&free_io_tasks);
}

void swap_in(pte_t *page_table_entry, uint32_t badvaddr)
{
    uint32_t address = page_table_entry->addr.swap_area_addr;
    int pm_index = pm_alloc();
    if(pm_index == -1)
    {
        swap_out();
        pm_index = pm_alloc();
    }

    //do_semaphore_down(&free_io_tasks);

    uint32_t PA = PM_BASE + pm_index*PM_SCHE_SIZE;
    unsigned char *buffer = (unsigned char *)(PA | 0xa0000000); // use uncached & unmapped segment

    //sdread(buffer, address, SWAP_AREA_UINT_SIZE);

    io_task.type = IO_TASK_READ;
    io_task.buffer = buffer;
    io_task.base = address;
    io_task.n = SWAP_AREA_UINT_SIZE;
    do_semaphore_up(&io_tasks);
    do_semaphore_down(&io_done);

    swap_area_free(address);

    page_table_entry->addr.PFN = ((uint32_t)(PM_BASE + pm_index*PM_SCHE_SIZE)) >> 12;
    page_table_entry->present = 1;

    pm_pid[pm_index] = current_running->pid;
    pm_vpn[pm_index] = badvaddr & 0xfffff000;

    //do_semaphore_up(&free_io_tasks);
}