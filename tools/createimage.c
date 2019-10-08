#include <assert.h>
#include <elf.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Elf32_Phdr *read_exec_file(FILE *opfile)
{
    Elf32_Ehdr ehdr;
    Elf32_Phdr *phdr;
    Elf32_Off phoff;  //bytes

    phdr = (Elf32_Phdr *)malloc(sizeof(Elf32_Phdr));
    fseek(opfile, 0, SEEK_SET);
    fread((void *)(&ehdr), sizeof(Elf32_Ehdr), 1, opfile);
    phoff = ehdr.e_phoff;
    fseek(opfile, phoff, SEEK_SET);
    fread((void *)phdr, sizeof(Elf32_Phdr), 1, opfile);

    return phdr;
}

uint32_t count_kernel_sectors(Elf32_Phdr *phdr)
{
    Elf32_Word filesz = phdr->p_memsz;  //bytes
    return (uint32_t)(filesz/512 + ((filesz%512) > 0));
}

void write_bootblock(FILE *image, FILE *file, Elf32_Phdr *phdr)
{
    Elf32_Off offset = phdr->p_offset;
    Elf32_Word filesz = phdr->p_filesz;
    void *buf;

    fseek(image, 0, SEEK_SET);
    fseek(file, offset, SEEK_SET);
    buf = malloc(filesz);
    fread(buf, filesz, 1, file);
    fwrite(buf, filesz, 1, image);
    free(buf);
}

void write_kernel(FILE *image, FILE *knfile, Elf32_Phdr *phdr)
{
    Elf32_Off offset = phdr->p_offset;
    Elf32_Word filesz = phdr->p_filesz;
    Elf32_Word memsz  = phdr->p_memsz;
    void *buf;
    int i;
    char zero_1byte = 0x00;

    fseek(image, 0x200, SEEK_SET);
    fseek(knfile, offset, SEEK_SET);
    buf = malloc(filesz);
    fread(buf, filesz, 1, knfile);
    fwrite(buf, filesz, 1, image);
    free(buf);
    //todo: what's the relationship between filesz and memsz
    for(i = 0; i < memsz - filesz; ++i)
        fwrite((void *)&zero_1byte, 1, 1, image);
}

void record_kernel_sectors(FILE *image, Elf32_Phdr *phdr_k)
{
    unsigned int ins;
    unsigned int kernelsz = phdr_k->p_memsz;

    unsigned int kernelsz_low = kernelsz & 0xffff;
    unsigned int kernelsz_hi  = (kernelsz >> 16) & 0xffff;

    //a080_0024: 3c060000, lui a2, 0
    //a080_0028: 24c60200, ori a2, 0
    ins = 0x3c060000 | kernelsz_hi;
    fseek(image, 0x24, SEEK_SET);
    fwrite(&ins, 4, 1, image);
    ins = 0x34c60000 | kernelsz_low;
    fseek(image, 0x28, SEEK_SET);
    fwrite(&ins, 4, 1, image);
}

void extent_opt(Elf32_Phdr *phdr_bb, Elf32_Phdr *phdr_k)
{
    unsigned int bootblock_size = phdr_bb->p_filesz;
    uint32_t kernel_sectors = count_kernel_sectors(phdr_k);

    printf("bootloader size: %d B\n", bootblock_size);
    printf("kernel size: %d%s KB\n", kernel_sectors/2, (kernel_sectors % 2) ? ".5" : "");

    if(kernel_sectors > 7*1024*2) // 7M
        printf("----------Error: too large kernel----------\n");
}

int main(int argc, char **argv)
{
    FILE *bl_file, *kernel_file, *image;
    Elf32_Phdr *phdr_bl, *phdr_kernel;

    bl_file = fopen("bootblock", "r");
        if(!bl_file)    goto err_openfile;
    kernel_file = fopen("main", "r");
        if(!kernel_file)    goto err_openfile;
    image = fopen("image", "w+");
        if(!image)    goto err_openfile;

    phdr_bl = read_exec_file(bl_file);
    phdr_kernel = read_exec_file(kernel_file);

    write_bootblock(image, bl_file, phdr_bl);
    write_kernel(image, kernel_file, phdr_kernel);
    record_kernel_sectors(image, phdr_kernel);

    if(argc > 1)
        if(strcmp(argv[1], "--extended") == 0)
            extent_opt(phdr_bl, phdr_kernel);

    free(phdr_bl);
    free(phdr_kernel);
    fclose(bl_file);
    fclose(kernel_file);
    return 0;

    err_openfile:
        printf("can't open file.\n");
        return 0;
}
