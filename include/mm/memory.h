#ifndef INCLUDE_MEMORY_H_
#define INCLUDE_MEMORY_H_

#include "type.h"
#include "slist.h"

#define TLB_ENTRY_NUMBER 32

/*
 * there's 32 MB memory in loongson 1C300
 * from 0x0000_0000 to 0x0200_0000 ( [a, b) )
 * before 0x0080_0000 is used (8MB)
 * the kernel is placed to 0x0080_0000, no more than 0x00f0_0000
 * so kernel should be less than 7MB
 * from 0x00fe_0000 to 0x0100_0000 is kernel stack (128KB)
 * every task possesses 8KB if MAX_TASK_NUM = 16
 * from 0x00fd_f000 to 0x00fe_0000 is user stack of main (4KB)
 * from 0x00fd_e000 to 0x00fd_f000 is outside call pointer(4KB)
 * from 0x00f0_0000 to 0x00fd_e000 is reserved (888KB)
 * from 0x0100_0000 to 0x0200_0000 is available to user processes (16MB)
 */

#define OUTSIDE_CALL_BASE 0xa0fde000
#define OUTSIDE_CALL_TOP  0xa0fdf000

#define KERNEL_STACK_TOP_ADDRESS  0xa1000000
#define KERNEL_STACK_BASE_ADDRESS 0xa0fe0000
#define KERNEL_STACK_SIZE         0x00002000

#define USER_STACK_SIZE_OF_MAIN 0x00001000

#define USER_STACK_TOP_ADDRESS  0x80000000
#define USER_STACK_BASE_ADDRESS 0x7fc00000
#define INITIAL_USER_STACK_SIZE 0x00002000

/*
 * 4KB per page
 * scheduling two pages together
 * so a pte manages 2 pages, 8KB
 */
union u_address
{
    uint32_t PFN; // physical frame number
    uint32_t swap_area_addr;
};

typedef struct pte
{
    // PTN(page table number) is equal to index
    union u_address addr;
    uint8_t valid;
    uint8_t present;
    //uint8_t control_bits;
} pte_t;

/*
 * 0.5K ptes per page table
 * every process possesses 2GB virtual memory
 * so a process possesses 0.5K pdes
 */
typedef struct pde
{
    // PDN(page directory number) is equal to index
    uint32_t pool_num; // page table pool number
    uint8_t valid;
    //uint8_t control_bits;
} pde_t;

#define PDE_NUMBER 512
typedef pde_t page_dir_t[PDE_NUMBER];

#define PTE_NUMBER 512
typedef pte_t page_table_t[PTE_NUMBER];

#define PAGE_TABLE_POOL_SIZE 20
#define PAGE_TABLE_POOL_SLIST_SIZE PAGE_TABLE_POOL_SIZE + 1
typedef page_table_t page_table_pool_t[PAGE_TABLE_POOL_SIZE];

/*
 * 16MB physical memory
 * schedule 8KB each time
 * so size of slist is 2K
 */
#define PM_SLIST_SIZE 2049 // physical memory slist size
                           // 2049
                           // 5
#define PM_BASE       0x01000000
#define PM_SCHE_SIZE  0x00002000 // scheduling size

extern page_table_pool_t page_table_pool;
extern node_t pt_pool_slist[PAGE_TABLE_POOL_SLIST_SIZE];

extern node_t pm_slist[PM_SLIST_SIZE];
extern uint32_t pm_pid[PM_SLIST_SIZE];
extern uint32_t pm_vpn[PM_SLIST_SIZE];

void page_dir_init(page_dir_t pd);
void page_table_init(page_table_t pt);
int pt_pool_alloc();
int pt_pool_free(int index);
int pm_alloc();
int pm_free(int index);

/*
 * 8MB swap area in sd card
 * because sectors before 0x700200 is reserved for kernel
 * so swap area is from 0x700200 to 0xf00200 (Byte)
 * swap unit is 8KB, same as scheduling unit of physical memory
 */

#define SWAP_AREA_SIZE 1025
extern node_t swap_area_slist[SWAP_AREA_SIZE];

#define SWAP_AREA_BASE      0x700200
#define SWAP_AREA_TOP       0xf00200
#define SWAP_AREA_UINT_SIZE 0x2000

uint32_t swap_area_alloc();
int swap_area_free(uint32_t address);

void swap_out();
void swap_in(pte_t *page_table_entry, uint32_t badvaddr);

void set_TLB_invalid(uint32_t EntryHi);

#endif
