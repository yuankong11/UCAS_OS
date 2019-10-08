#include "sched.h"
#include "elf32.h"

uint8_t run_buf[2047];

void sw_run()
{
    Elf32_Ehdr *ehdr = (Elf32_Ehdr *)run_buf;
    Elf32_Off phoff = ehdr->e_phoff;
    Elf32_Phdr *phdr = (Elf32_Phdr *)(run_buf + phoff);
    Elf32_Off offset = phdr->p_offset;
    Elf32_Word filesz = phdr->p_filesz;
    memcopy((void *)0, (void *)(run_buf + offset), filesz);
    asm("jr $0\n");
}

task_info_t run = {(uint32_t)&sw_run, USER_PROCESS, 1, "run"};
task_info_t *info_run = &run;