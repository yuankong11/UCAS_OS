#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>
#include <elf.h>

int main()
{
    struct stat st;
    off_t size;
    stat("bonus_out", &st);
    size = st.st_size;
    printf("size: %ld\n", size);

    FILE *file_in, *file_out;
    file_in = fopen("bonus_out", "r");
    file_out = fopen("pkt.txt", "w");
    Elf32_Phdr phdr;
    Elf32_Ehdr ehdr;
    Elf32_Off phoff;

    fseek(file_in, 0, SEEK_SET);
    fread((void *)&ehdr, sizeof(Elf32_Ehdr), 1, file_in);
    phoff = ehdr.e_phoff;
    fseek(file_in, phoff, SEEK_SET);
    fread((void *)&phdr, sizeof(Elf32_Phdr), 1, file_in);

    Elf32_Off offset = phdr.p_offset;
    Elf32_Word filesz = phdr.p_filesz;
    printf("offset: %d, filesz: %d\n", offset, filesz);
    fseek(file_in, 0, SEEK_SET);
    fseek(file_out, 0, SEEK_SET);
    int valid_size = offset + filesz;
    int i, t;
    char comma = ',';
    unsigned char buf[4];
    for(i = 0; i < valid_size; ++i)
    {
        fread(&buf[0], 1, 1, file_in);
        t = sprintf(buf, "%d", buf[0]);
        fwrite(buf, t, 1, file_out);
        fwrite(&comma, 1, 1, file_out);
    }
    printf("valid size: %d\n", valid_size);

    fclose(file_in);
    fclose(file_out);

    return 0;
}