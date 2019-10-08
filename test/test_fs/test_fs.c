#include "fs.h"
#include "stdio.h"
#include "string.h"
#include "test_fs.h"
#include "syscall.h"

static char buff[64];

void test_fs1(void)
{
    int i, j;
    int fd = sys_fopen("1.txt", O_RDWR);
    if(fd == -1)
    {
        sys_move_cursor(0, 0);
        printf("Error: 1.txt does not exist.");
        sys_exit();
    }

    for (i = 0; i < 10; i++)
    {
        sys_fwrite(fd, "hello world!\n", 13);
    }

    sys_move_cursor(0, 0);
    sys_fseek(fd, 0);
    for (i = 0; i < 10; i++)
    {
        sys_fread(fd, buff, 13);
        for (j = 0; j < 13; j++)
        {
            printf("%c", buff[j]);
        }
    }

    sys_fclose(fd);
    sys_exit();
}