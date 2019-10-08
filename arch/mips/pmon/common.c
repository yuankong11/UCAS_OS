#include "common.h"
#include "string.h"
#include "sched.h"
#include "type.h"

void port_write_ch(char ch)
{
    typedef void (*FUNC_POINT)(char s);
    volatile FUNC_POINT  _printch = (FUNC_POINT)0x8007ba00;
    _printch(ch);
}

void port_write(char *str)
{
    typedef void (*FUNC_POINT)(char *s);
    volatile FUNC_POINT  _printstr = (FUNC_POINT)0x8007b980;
    _printstr(str);
}

void read_uart_ch(void)
{
    char ch = 0;
    volatile unsigned char *read_port   = (unsigned char *)(0xbfe48000 + 0x00);
    volatile unsigned char *status_port = (unsigned char *)(0xbfe48000 + 0x05);

    if ((*status_port & 0x01)) // there's data in fifo
    {
        ch = *read_port;
    }

    current_running->user_regs_context.regs[2] = (uint32_t)ch;
}

void *memcopy(void *s1, const void *s2, int n)
{
        const char *f = s2;
        char *t = s1;

        if(f < t)
        {
            f += n;
            t += n;
            while (n-- > 0)
                *--t = *--f;
        }
        else
        {
            while (n-- > 0)
                *t++ = *f++;
        }
        return s1;
}

unsigned int write_block(unsigned long int write_addr, unsigned char *write_buffer)
{
    //a block is 512 Bytes
    typedef unsigned char (*FUNC_POINT1)(unsigned char value);  //flash_writeb_cmd
    typedef void (*FUNC_POINT2)(unsigned int IOData);           //SD_2Byte_Write
    typedef void (*FUNC_POINT3)(unsigned int IOData);           //SD_Write
    typedef unsigned short (*FUNC_POINT4)(void);                //SD_Read
    typedef unsigned int (*FUNC_POINT5)(unsigned int CMDIndex, unsigned long CMDArg, unsigned int ReaType, unsigned int CSLowRSV);  //SD_CMD_Write

    FUNC_POINT1 flash_writeb_cmd = (FUNC_POINT1)0x800794c0;
    FUNC_POINT2 SD_2Byte_Write   = (FUNC_POINT2)0x800794f8;
    FUNC_POINT3 SD_Write         = (FUNC_POINT3)0x80079528;
    FUNC_POINT4 SD_Read          = (FUNC_POINT4)0x80079564;
    FUNC_POINT5 SD_CMD_Write     = (FUNC_POINT5)0x80079588;

    unsigned int temp, Response, max_times;
    max_times = 10;

    for(temp = 0; temp < max_times; ++temp)
    {
        /* Send CMD24 */
        Response = SD_CMD_Write(24, write_addr, 1, 1);
        if (Response == 0xff00)
            break;
    }

    /* Provide 8 extra clock after CMD response */
    flash_writeb_cmd(0xff);

    /* Send Start Block Token */
    SD_Write(0x00fe);
    for(temp = 0; temp < 256; ++temp)
    {
        /* Data Block */
        SD_2Byte_Write(((write_buffer[2*temp])<<8) | (write_buffer[2*temp+1]));
    }
    /* Send 2 Bytes CRC */
    SD_2Byte_Write(0xffff);

    Response = SD_Read();
    while(SD_Read()!=0xffff)
        ;

    //set_cs(1);
    *(volatile unsigned char *)( ((void *)(0xa0000000 | (unsigned int)(0x1fe80000+0x05))) ) = (0xFF);

    /* Provide 8 extra clock after data response */
    flash_writeb_cmd(0xff);

    return Response;
}

int isQEMU()
{
    unsigned long *tempaddr = (unsigned long *)0x8005f65c;
    if(*(tempaddr + 0) == 0x27bdffa8)
        return 1;
    else
        return 0;
}

int sdwrite(unsigned char *buf, unsigned int base, int n)
{
    unsigned long *tempaddr = (unsigned long *)0x8005f65c;
    if(isQEMU())
    {
        /*QEMU sdwrite*/
        typedef int (*FUNC_POINT)(unsigned int base, int n, unsigned char *buffer);
        FUNC_POINT new_sdcard_write = (FUNC_POINT)0x8007aba4;
        new_sdcard_write(base, n, buf);
    }
    else
    {
        /*LS1C sdwrite*/
        int i;

        unsigned int pos = base;
        unsigned int left = n;
        unsigned int once;
        // todo: conflict
        unsigned char *indexbuf = (unsigned char *)(0xa0100400 + 0x0);

        while(left)
        {
            once = (512 - (pos&511)) < (left) ? (512 - (pos&511)) : (left);
            memcopy(indexbuf+(pos&511), buf, once);

            write_block(pos>>9, indexbuf);

            buf += once;
            pos += once;
            left -= once;
        }
        base = pos;
    }

    return (n);
}

int sdread(unsigned char *buf, unsigned int base, int n)
{
    unsigned long *tempaddr = (unsigned long *)0x8005f65c;
    if(isQEMU())
    {
        /*QEMU sdread*/
        typedef int (*FUNC_POINT)(unsigned char *buffer, unsigned int base, int n);
        FUNC_POINT new_sdcard_read = (FUNC_POINT)0x8007b1cc;
        new_sdcard_read(buf, base, n);
    }
    else
    {
        /*LS1C sdread*/
        int i;
        typedef unsigned int (*FUNC_POINT)(unsigned long int ByteAddress, unsigned char *ReadBuffer);
        FUNC_POINT Read_Single_Block = (FUNC_POINT)0x80079cd8;

        unsigned int pos = base;
        unsigned int left = n;
        unsigned int once;
        // todo: conflict
        unsigned char *indexbuf = (unsigned char *)(0xa0100200 + 0x0);

        while(left)
        {
            Read_Single_Block(pos>>9, indexbuf);
            once = (512 - (pos&511)) < (left) ? (512 - (pos&511)) : (left);

            memcopy(buf, indexbuf + (pos&511), once);

            buf += once;
            pos += once;
            left -= once;
        }
        base = pos;
    }

    return (n);
}