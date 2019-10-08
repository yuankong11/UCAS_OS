#include "software.h"
#include "stdio.h"
#include "screen.h"
#include "syscall.h"
#include "string.h"
#include "debug.h"
#include "test.h"
#include "fs.h"
#include "common.h"
#include "elf32.h"

int atoi(char *s)
{
    int res = 0, i;
    for(i = 0; s[i]; ++i)
        if(s[i] >= '0' && s[i] <= '9')
            res = res*10 + s[i] - '0';
        else return -1;
    return res;
}

#define COMMEND_NUM 14

static char *commend[COMMEND_NUM] =
{
    "ps",
    "clear",
    "exec",
    "kill",
    "mkfs",
    "statfs",
    "currdir",
    "mkdir",
    "rmdir",
    "ls",
    "cd",
    "touch",
    "cat",
    "run"
};

static int commend_num = COMMEND_NUM;
static char input_buffer[SCREEN_WIDTH];
static int  buffer_index;

static void parse_commend()
{
    int i;
    char *commend_part = input_buffer;
    char *arg_string;
    int arg_int = 0;
    int separator_position = separate(input_buffer, ' ');

    // todo: wrong args
    // todo: multi args
    if(separator_position != buffer_index)
    {
        arg_string = input_buffer + separator_position + 1;
        // todo: empty arg_int
        input_buffer[separator_position] = '\0';
        arg_int = atoi(arg_string);
    }

    for(i = 0; i < commend_num; ++i)
    {
        if(strcmp(commend[i], commend_part) == 0)
        {
            switch(i)
            {
                // todo: useless argument in some commend
                case 0:
                {
                    sys_show_process(); break;
                }
                case 1:
                {
                    if(arg_int == 0)
                    {
                        sys_clear(SCREEN_HEIGHT/2+1, SCREEN_HEIGHT-1);
                        sys_move_cursor(0, SCREEN_HEIGHT/2+1); // reset cursor
                    }
                    else if(arg_int == 1)
                    {
                        sys_clear(0, SCREEN_HEIGHT/2-1);
                    }
                    else
                    {
                        sys_clear(0, SCREEN_HEIGHT/2-1);
                        sys_clear(SCREEN_HEIGHT/2+1, SCREEN_HEIGHT-1);
                        sys_move_cursor(0, SCREEN_HEIGHT/2+1); // reset cursor
                    }
                    break;
                }
                case 2:
                {
                    if(arg_int > test_tasks_num)
                        printf("Error: too large index.\n");
                    else if(arg_int < 1)
                        printf("Error: wrong index.\n");
                    else if(!sys_spawn(test_tasks[arg_int-1]))
                        printf("Error: too many tasks.\n");
                    break;
                }
                case 3:
                {
                    if(!sys_kill(arg_int))
                        printf("Illegal pid.\n");
                    break;
                }
                case 4:
                {
                    sys_mkfs();
                    // todo: change cd here
                    sys_mount();
                    break;
                }
                case 5:
                {
                    sys_statfs();
                    break;
                }
                case 6:
                {
                    char *s = sys_get_curr_dir_name();
                    printf("%s\n", s);
                    break;
                }
                case 7:
                {
                    sys_mkdir(arg_string);
                    break;
                }
                case 8:
                {
                    sys_rmdir(arg_string);
                    break;
                }
                case 9:
                {
                    sys_ls();
                    break;
                }
                case 10:
                {
                    sys_cd(arg_string);
                    break;
                }
                case 11:
                {
                    sys_touch(arg_string);
                    break;
                }
                case 12:
                {
                    // todo: single syscall run faster
                    int fd = sys_fopen(arg_string, O_RD);
                    if(fd == -1)
                    {
                        printf("Error: open file failed.\n");
                        break;
                    }
                    char buf[513];
                    int size;
                    buf[512] = '\0';
                    while((size = sys_fread(fd, buf, SECTOR_SIZE)) == SECTOR_SIZE)
                    {
                        //printf("size: %d\n", size);
                        printf("%s", buf);
                    }
                    buf[size] = '\0';
                    //printf("size: %d\n", size);
                    printf("%s", buf);
                    break;
                }
                case 13:
                {
                    int fd = sys_fopen(arg_string, O_RD);
                    if(fd == -1)
                    {
                        printf("Error: no such executable file.\n");
                        break;
                    }
                    sys_fread(fd, run_buf, 2047);
                    if(run_buf[0] != 127 || run_buf[1] != 69 || run_buf[2] != 76 || run_buf[3] != 70)
                    {
                        printf("Error: not a ELF file.\n");
                        break;
                    }
                    sys_spawn(info_run);
                    break;
                }
                default:
                {
                    panic("Parse commend error.");
                }
            }
            return;
        }
    }
    if(buffer_index > 0)
        printf("No such commend.\n");
}

static char *prefix = "> SBZeng@UCAS_OS: ";
static int prefix_length;;

static void sw_shell()
{
    int i;
    char c;

    sys_move_cursor(0, SCREEN_HEIGHT/2);

    for(i = 0; i < SCREEN_WIDTH; ++i)
        printf("-");
    printf("%s", prefix);
    prefix_length = strlen(prefix);

    while (1)
    {
        c = sys_read();
        switch(c)
        {
            case 0: break; // no char in
            case 10: case 13: // LF || CR
            {
                printf("%c", c);
                input_buffer[buffer_index] = '\0';
                parse_commend();
                buffer_index = 0;
                printf("%s", prefix);
                break;
            }
            case 8: case 127:  // BACKSPACE || DEL
            {
                if(buffer_index > 0)
                {
                    printf("%c", c);
                    --buffer_index;
                }
                break;
            }
            default:
            {
                printf("%c", c);
                if(buffer_index == SCREEN_WIDTH - prefix_length)
                {
                    buffer_index = 0;
                    printf("No such commend.\n");
                    printf("%s", prefix);
                }
                input_buffer[buffer_index++] = c;
            }
        }
    }
}

task_info_t shell = {(uint32_t)&sw_shell, USER_PROCESS, 1, "shell"};
task_info_t *info_shell = &shell;