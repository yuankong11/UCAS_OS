#include "fs.h"
#include "type.h"
#include "common.h"
#include "stdio.h"
#include "screen.h"
#include "debug.h"
#include "string.h"
#include "sched.h"

fd_t fd_array[FD_ARRAY_SIZE];
node_t fd_array_slist[FD_ARRAY_SLIST_SIZE];

super_block_t super_block;
dir_t current_dir;
uint32_t current_dir_pointer;

uint32_t imap[128];
uint32_t smap[128];
uint32_t smap_index; // show where is smap at, sectors

void mkfs()
{
    int i;
    printks("[FS] Initializing file system...\n");

    // super block
    printks("[FS] Setting super block...\n");
    {
        super_block_t s;
        s.imap_offset = 1;
        s.smap_offset = 2;
        s.iarray_offset = 258;
        s.darray_offset = 1024;
        s.inode_size = 64;
        s.directory_size = 512;
        s.inode_number = INODE_ITEM_NUM;
        s.data_number = DATA_ITEM_NUM;
        s.data_used = 1;  // for root
        s.inode_used = 1; // for root
        s.reserved[78] = MAGIC_NUMBER;
        sdwrite((uint8_t *)&s, FS_BASE, SECTOR_SIZE);
    }

    // inode map & sector map
    printks("[FS] Setting inode map & sector map...\n");
    {
        uint32_t buf[128] = {0};
        buf[0] = 0x1;
        sdwrite((uint8_t *)buf, IMAP_ADDR, SECTOR_SIZE);
        sdwrite((uint8_t *)buf, SMAP_ADDR, SECTOR_SIZE);
        buf[0] = 0;
        uint32_t addr = SMAP_ADDR + SECTOR_SIZE;
        for(i = 0; i < 255; ++i, addr += SECTOR_SIZE)
            sdwrite((uint8_t *)buf, addr, SECTOR_SIZE);
    }

    // root
    printks("[FS] Setting root directory...\n");
    {
        inode_t inode[8];
        inode[0].type = FILE_DIR;
        inode[0].links = 1;
        inode[0].size = 512;
        inode[0].size_allocated = 512;
        inode[0].pointers[0] = DARRAY_ADDR;
        sdwrite((uint8_t *)inode, IARRAY_ADDR, SECTOR_SIZE);
        dir_t dir_root;
        for(i = 0; i < MAX_DIR_ITEM_NUM; ++i)
            dir_root.map[i].valid = 0;
        dir_root.fpointer = DARRAY_ADDR;
        strcpy(dir_root.name, "/");
        sdwrite((uint8_t *)&dir_root, DARRAY_ADDR, SECTOR_SIZE);
    }

    printks("[FS] Initialization finished.\n");
}

void mount()
{
    printks("[FS] File system mounting...\n");
    sdread((uint8_t *)&super_block, FS_BASE, SECTOR_SIZE);
    sdread((uint8_t *)&current_dir, DARRAY_ADDR, SECTOR_SIZE);
    current_dir_pointer = DARRAY_ADDR;
    sdread((uint8_t *)imap, IMAP_ADDR, SECTOR_SIZE);
    smap_index = 0;
    sdread((uint8_t *)smap, SMAP_ADDR, SECTOR_SIZE);
    printks("[FS] File system mounted.\n");
}

void statfs()
{
    super_block_t s;
    sdread((uint8_t *)&s, FS_BASE, SECTOR_SIZE);
    printks("[FS] used inode items: %d/%d,  used data blocks: %d/%d.\n",
             s.inode_used, s.inode_number, s.data_used, s.data_number);
    printks("[FS] magic number: 0x%08X,  start address: 0x%08x.\n",
             s.reserved[78], FS_BASE);
    printks("[FS] inode  map offset: %d sector(s),  occupied: %d sector(s)\n",
             s.imap_offset, 1);
    printks("[FS] sector map offset: %d sector(s),  occupied: %d sector(s)\n",
             s.smap_offset, 256);
    printks("[FS] inode offset: %d sector(s),  data offset: %d sector(s).\n",
             s.iarray_offset, s.darray_offset);
    printks("[FS] inode size: %d byte(s),  directory size: %d byte(s).\n",
             s.inode_size, s.directory_size);
}

static int iarray_alloc()
{
    int i, j;
    uint32_t t;
    for(i = 0; i < 128; ++i)
    {
        t = imap[i];
        for(j = 0; (t & 0x1) && j < 32; t >>= 1, ++j)
            ;
        if(j != 32)
            break;
    }
    if(i == 128 && j == 32)
        return -1;
    else
    {
        imap[i] |= (1 << j);
        sdwrite((uint8_t *)imap, IMAP_ADDR, SECTOR_SIZE);
        super_block.inode_used += 1;
        sdwrite((uint8_t *)&super_block, FS_BASE, SECTOR_SIZE);
        return i*32 + j;
    }
}

static void iarray_free(int ino)
{
    int i = ino/32;
    int j = ino - i*32;
    imap[i] &= ~(1 << j);
    sdwrite((uint8_t *)imap, IMAP_ADDR, SECTOR_SIZE);
    super_block.inode_used -= 1;
    sdwrite((uint8_t *)&super_block, FS_BASE, SECTOR_SIZE);
}

static int darray_alloc()
{
    int i, j;
    uint32_t t;
    while(smap_index < 256)
    {
        for(i = 0; i < 128; ++i)
        {
            t = smap[i];
            for(j = 0; (t & 0x1) && j < 32; t >>= 1, ++j)
                ;
            if(j != 32)
                break;
        }
        if(i == 128 && j == 32)
        {
            ++smap_index;
            sdread((uint8_t *)smap, SMAP_ADDR + smap_index*SECTOR_SIZE, SECTOR_SIZE);
        }
        else
        {
            smap[i] |= (1 << j);
            sdwrite((uint8_t *)smap, SMAP_ADDR + smap_index*SECTOR_SIZE, SECTOR_SIZE);
            super_block.data_used += 1;
            sdwrite((uint8_t *)&super_block, FS_BASE, SECTOR_SIZE);
            return smap_index*512*8 + i*32 + j;
        }
    }
    return -1;
}

static void darray_free(int dno)
{
    int si = dno/(512*8);
    dno -= si*512*8;
    int i = dno/32;
    int j = dno - i*32;
    smap_index = si;
    sdread((uint8_t *)smap, SMAP_ADDR + smap_index*SECTOR_SIZE, SECTOR_SIZE);
    smap[i] &= ~(1 << j);
    sdwrite((uint8_t *)smap, SMAP_ADDR + smap_index*SECTOR_SIZE, SECTOR_SIZE);
    super_block.data_used -= 1;
    sdwrite((uint8_t *)&super_block, FS_BASE, SECTOR_SIZE);
}

int is_file_exist(char *s)
{
    // if exist, return its ino
    // else return 0
    int i;
    for(i = 0; i < MAX_DIR_ITEM_NUM; ++i)
        if(current_dir.map[i].valid)
            if(strcmp(current_dir.map[i].name, s) == 0)
                return current_dir.map[i].ino;
    return 0;
}

uint32_t parse_path(char *s)
{
    // return ino of c of "/a/b/c"
    // return 0 if s == "\\"
    // if no found, return -1
    int i;
    char *s0;
    int pos, found;
    uint32_t ino, p;
    inode_t inode[8];

    if(strcmp(s, "\\") == 0)
        return 0;

    dir_t dir;
    sdread((uint8_t *)&dir, DARRAY_ADDR, SECTOR_SIZE);
    ++s;

    for(pos = separate(s, '\\'); pos != -1; pos = separate(s, '\\'))
    {
        // change input buffer in shell, inappropriate, but correct now
        s[pos] = '\0';
        s0 = s;
        s = s + pos + 1;
        found = 0;
        for(i = 0; i < MAX_DIR_ITEM_NUM; ++i)
        {
            if(dir.map[i].valid)
                if(strcmp(dir.map[i].name, s0) == 0)
                {
                    ino = dir.map[i].ino;
                    int ino_group = ino/8;
                    int ino_remain = ino - ino_group*8;
                    sdread((uint8_t *)inode, IARRAY_ADDR + ino_group*SECTOR_SIZE, SECTOR_SIZE);
                    if(inode[ino_remain].type != FILE_DIR)
                        return -1;
                    p = inode[ino_remain].pointers[0];
                    sdread((uint8_t *)&dir, p, SECTOR_SIZE);
                    found = 1;
                    break;
                }
        }
        if(found == 0)
            return -1;
    }
    for(i = 0; i < MAX_DIR_ITEM_NUM; ++i)
    {
        if(dir.map[i].valid)
            if(strcmp(dir.map[i].name, s) == 0)
            {
                // todo: match_item
                ino = dir.map[i].ino;
                return ino;
            }
    }
    return -1;
}

void mkdir(char *s)
{
    int i;

    if(strlen(s) > MAX_NAME_LENTH)
    {
        printks("Error: too long name.\n");
        return;
    }

    if(is_file_exist(s))
    {
        printks("Error: file exists.\n");
        return;
    }

    for(i = 0; current_dir.map[i].valid && i < MAX_DIR_ITEM_NUM; ++i)
        ;
    if(i == MAX_DIR_ITEM_NUM)
    {
        printks("Too many directory entrys.\n");
        return;
    }
    current_dir.map[i].valid = 1;
    int ino = iarray_alloc();
    if(ino == -1)
    {
        printks("Error: insufficient inode.\n");
        return;
    }
    current_dir.map[i].ino = ino;
    strcpy(current_dir.map[i].name, s);
    sdwrite((uint8_t *)&current_dir, current_dir_pointer, SECTOR_SIZE);

    inode_t inode[8];
    int ino_group = ino/8;
    ino = ino - ino_group*8;
    sdread((uint8_t *)inode, IARRAY_ADDR + ino_group*SECTOR_SIZE, SECTOR_SIZE);
    inode[ino].type = FILE_DIR;
    inode[ino].links = 1;
    inode[ino].size = 512;
    inode[ino].size_allocated = 512;
    uint32_t dno = darray_alloc();
    if(dno == -1)
    {
        printks("Error: insufficient data block.\n");
        return;
    }
    uint32_t p = DARRAY_ADDR + dno*SECTOR_SIZE;
    inode[ino].pointers[0] = p;
    sdwrite((uint8_t *)inode, IARRAY_ADDR + ino_group*SECTOR_SIZE, SECTOR_SIZE);

    dir_t d;
    for(i = 0; i < MAX_DIR_ITEM_NUM; ++i)
        d.map[i].valid = 0;
    d.fpointer = current_dir_pointer;
    strcpy(d.name, s);
    sdwrite((uint8_t *)&d, p, SECTOR_SIZE);
}

void rmdir(char *s)
{
    int i;
    for(i = 0; i < MAX_DIR_ITEM_NUM; ++i)
    {
        if(current_dir.map[i].valid)
            if(strcmp(current_dir.map[i].name, s) == 0)
            {
                uint32_t ino = current_dir.map[i].ino;
                inode_t inode[8];
                int ino_group = ino/8;
                int ino_remain = ino - ino_group*8;
                sdread((uint8_t *)inode, IARRAY_ADDR + ino_group*SECTOR_SIZE, SECTOR_SIZE);
                if(inode[ino_remain].type != FILE_DIR)
                {
                    printks("Error: not a directory.\n");
                    return;
                }
                uint32_t p = inode[ino_remain].pointers[0];
                dir_t d;
                sdread((uint8_t *)&d, p, SECTOR_SIZE);
                int j;
                for(j = 0; j < MAX_DIR_ITEM_NUM; ++j)
                    if(d.map[j].valid)
                    {
                        printks("Error: directory not empty.\n");
                        return;
                    }
                iarray_free(ino);
                darray_free((p - DARRAY_ADDR)/SECTOR_SIZE);
                current_dir.map[i].valid = 0;
                sdwrite((uint8_t *)&current_dir, current_dir_pointer, SECTOR_SIZE);
                return;
            }
    }
    printks("Error: no such directory.\n");
}

void ls()
{
    int i, j = 0;
    printks(".  ..\n");
    for(i = 0; i < MAX_DIR_ITEM_NUM; ++i)
    {
        if(current_dir.map[i].valid)
        {
            printks("%s  ", current_dir.map[i].name);
            if(j == 4)
            {
                printks("\n");
                j = 0;
            }
            else
                ++j;
        }
    }
    if(j != 0)
        printks("\n");
}

void cd(char *s)
{
    int i;
    if(strcmp("..", s) == 0)
    {
        if(current_dir_pointer == DARRAY_ADDR) // in root
            return;
        uint32_t p = current_dir.fpointer;
        current_dir_pointer = p;
        sdread((uint8_t *)&current_dir, p, SECTOR_SIZE);
        return;
    }

    if(strcmp(".", s) == 0)
        return;

    uint32_t ino = -1;
    if(s[0] == '\\')
    {
        ino = parse_path(s);
    }
    else
    {
        for(i = 0; i < MAX_DIR_ITEM_NUM; ++i)
            if(current_dir.map[i].valid)
                if(strcmp(current_dir.map[i].name, s) == 0)
                {
                    ino = current_dir.map[i].ino;
                    break;
                }
    }

    if(ino == -1)
        printks("Error: not such directory.\n");
    else
    {
        inode_t inode[8];
        int ino_group = ino/8;
        int ino_remain = ino - ino_group*8;
        sdread((uint8_t *)inode, IARRAY_ADDR + ino_group*SECTOR_SIZE, SECTOR_SIZE);
        if(inode[ino_remain].type != FILE_DIR)
        {
            printks("Error: not a directory.\n");
            return;
        }
        uint32_t p = inode[ino_remain].pointers[0];
        sdread((uint8_t *)&current_dir, p, SECTOR_SIZE);
        current_dir_pointer = p;
    }
}

void get_curr_dir_name()
{
    current_running->user_regs_context.regs[2] = (uint32_t)current_dir.name;
}

void touch(char *s)
{
    int i;

    if(strlen(s) > MAX_NAME_LENTH)
    {
        printks("Error: too long name.\n");
        return;
    }

    if(is_file_exist(s))
    {
        printks("Error: file exists.\n");
        return;
    }

    for(i = 0; current_dir.map[i].valid && i < MAX_DIR_ITEM_NUM; ++i)
        ;
    if(i == MAX_DIR_ITEM_NUM)
    {
        printks("Too many directory entrys.\n");
        return;
    }
    current_dir.map[i].valid = 1;
    int ino = iarray_alloc();
    if(ino == -1)
    {
        printks("Error: insufficient inode.\n");
        return;
    }
    //printks("i: %d, ino: %d\n", i, ino);
    current_dir.map[i].ino = ino;
    strcpy(current_dir.map[i].name, s);
    sdwrite((uint8_t *)&current_dir, current_dir_pointer, SECTOR_SIZE);

    inode_t inode[8];
    int ino_group = ino/8;
    ino = ino - ino_group*8;
    sdread((uint8_t *)inode, IARRAY_ADDR + ino_group*SECTOR_SIZE, SECTOR_SIZE);
    inode[ino].type = FILE_NORMAL;
    inode[ino].links = 1;
    inode[ino].size = 0;
    inode[ino].size_allocated = 0;
    sdwrite((uint8_t *)inode, IARRAY_ADDR + ino_group*SECTOR_SIZE, SECTOR_SIZE);
}

static inline int fd_array_alloc()
{
    return slist_alloc(fd_array_slist) - 1;
}

static inline int fd_array_free(int index)
{
    return slist_free(fd_array_slist, index + 1, FD_ARRAY_SLIST_SIZE);
}

void fopen(char *s, int access)
{
    // todo: parse path here

    int ino = is_file_exist(s);
    if(!ino)
    {
        current_running->user_regs_context.regs[2] = (uint32_t)-1;
        return;
    }

    inode_t inode[8];
    int ino_group = ino/8;
    ino = ino - ino_group*8;
    sdread((uint8_t *)inode, IARRAY_ADDR + ino_group*SECTOR_SIZE, SECTOR_SIZE);

    if(inode[ino].type != FILE_NORMAL)
    {
        current_running->user_regs_context.regs[2] = (uint32_t)-1;
        return;
    }

    int fd = fd_array_alloc();
    //printks("fd: %d\n", fd);
    if(fd == -1)
    {
        current_running->user_regs_context.regs[2] = (uint32_t)-1;
        return;
    }

    fd_array[fd].access = access;
    fd_array[fd].ino = ino;
    fd_array[fd].offset = 0;
    fd_array[fd].size = inode[ino].size;

    current_running->user_regs_context.regs[2] = (uint32_t)fd;
}

void fclose(int fd)
{
    if(fd == -1)
    {
        current_running->user_regs_context.regs[2] = (uint32_t)0;
        return;
    }
    int ret = fd_array_free(fd);
    current_running->user_regs_context.regs[2] = (uint32_t)ret;
}

static uint32_t offset_to_pointer(inode_t *inode, uint32_t offset, uint32_t *offset_remain)
{
    *offset_remain = offset - offset/512*512;
    offset /= 512;
    int seg = (offset >= 10) + (offset >= 138) + (offset >= 16522) + (offset >= 2113674);
    uint32_t d[128];
    uint32_t p;
    switch(seg)
    {
        case 0:
        {
            return inode->pointers[offset];
        }
        case 1:
        {
            offset -= 10;
            p = inode->pointers[10];
            sdread((uint8_t *)d, p, SECTOR_SIZE);
            return d[offset];
        }
        case 2:
        {
            offset -= 138;
            int i = offset/128;
            offset -= i*128;
            p = inode->pointers[11];
            sdread((uint8_t *)d, p, SECTOR_SIZE);
            p = d[i];
            sdread((uint8_t *)d, p, SECTOR_SIZE);
            return d[offset];
        }
        case 3:
        {
            offset -= 16522;
            int i = offset/(128*128);
            offset -= i*128*128;
            p = inode->pointers[12];
            sdread((uint8_t *)d, p, SECTOR_SIZE);
            p = d[i];
            sdread((uint8_t *)d, p, SECTOR_SIZE);
            i = offset/128;
            offset -= i*128;
            p = d[i];
            sdread((uint8_t *)d, p, SECTOR_SIZE);
            return d[offset];
        }
        default: panic("Wrong seg.");
    }
}

void fread(int fd, uint8_t *buf, uint32_t size)
{
    if(fd == -1)
    {
        current_running->user_regs_context.regs[2] = (uint32_t)0;
        return;
    }

    if(fd_array[fd].access != O_RDWR && fd_array[fd].access != O_RD)
    {
        current_running->user_regs_context.regs[2] = (uint32_t)0;
        return;
    }

    inode_t inode[8];
    int ino = fd_array[fd].ino;
    //printks("ino: %d  offset:%d  ", ino, fd_array[fd].offset);
    int ino_group = ino/8;
    ino = ino - ino_group*8;
    sdread((uint8_t *)inode, IARRAY_ADDR + ino_group*SECTOR_SIZE, SECTOR_SIZE);

    int remain = fd_array[fd].size - fd_array[fd].offset;
    int size_remain = (remain < size) ? remain : size;
    //printks("si:%d sr: %d\n", fd_array[fd].size, size_remain);
    uint32_t read_buf[128];
    uint32_t p, offset_remain;
    int read_size;
    int total_size = size_remain;
    while(size_remain)
    {
        // read here
        p = offset_to_pointer(&inode[ino], fd_array[fd].offset, &offset_remain);
        //printks("p: %08x  or:%d  ", p, offset_remain);
        read_size = (size_remain < SECTOR_SIZE - offset_remain) ? size_remain : SECTOR_SIZE - offset_remain;
        //printks("rs: %d\n", read_size);
        sdread((uint8_t *)read_buf, p, SECTOR_SIZE);
        //printks("%08x %08x %d\n", read_buf, (uint8_t *)read_buf + offset_remain, read_size);
        memcopy((void *)buf, (void *)((uint8_t *)read_buf + offset_remain), read_size);
        buf += read_size;
        size_remain -= read_size;
        fd_array[fd].offset += read_size;
    }
    current_running->user_regs_context.regs[2] = (uint32_t)total_size;
}

void fwrite(int fd, uint8_t *buf, uint32_t size)
{
    if(fd == -1)
    {
        current_running->user_regs_context.regs[2] = (uint32_t)0;
        return;
    }

    if(fd_array[fd].access != O_RDWR && fd_array[fd].access != O_WR)
    {
        current_running->user_regs_context.regs[2] = (uint32_t)0;
        return;
    }

    inode_t inode[8];
    int ino = fd_array[fd].ino;
    //printks("ino: %d  offset:%d  ", ino, fd_array[fd].offset);
    int ino_group = ino/8;
    ino = ino - ino_group*8;
    sdread((uint8_t *)inode, IARRAY_ADDR + ino_group*SECTOR_SIZE, SECTOR_SIZE);

    uint32_t write_buf[128];
    uint32_t p;
    uint32_t pi;
    uint32_t offset_remain;
    int read_size;
    int size_written = 0;
    int remain;
    // todo: file size caculating error when write after write
    while(size)
    {
        // size < 512
        remain = inode[ino].size_allocated - fd_array[fd].offset;
        //printks("remain: %d\n", remain);
        if(remain >= size)
        {
            p = offset_to_pointer(&inode[ino], fd_array[fd].offset, &offset_remain);
            //printks("p: %08x  ", p);
            read_size = fd_array[fd].offset - (fd_array[fd].offset/512)*512;
            //printks("rs: %d  ", read_size);
            sdread((uint8_t *)write_buf, p, SECTOR_SIZE);
            int t = 512 - read_size;
            t = (size < t) ? size : t;
            memcopy((void *)((uint8_t *)write_buf + read_size), (void *)buf, t);
            sdwrite((uint8_t *)write_buf, p, SECTOR_SIZE);
            //printks("t: %d\n", t);
            inode[ino].size += t;
            fd_array[fd].offset += t;
            fd_array[fd].size += t;
            size -= t;
            size_written += t;
        }
        else
        {
            pi = inode[ino].size_allocated/512;
            //printks("pi: %d  ", pi);
            uint32_t dno = darray_alloc();
            //printks("dno: %d\n", dno);
            if(dno == -1)
            {
                sdwrite((uint8_t *)inode, IARRAY_ADDR + ino_group*SECTOR_SIZE, SECTOR_SIZE);
                current_running->user_regs_context.regs[2] = (uint32_t)size_written;
                return;
            }
            p = DARRAY_ADDR + dno*SECTOR_SIZE;
            // todo: overflow
            inode[ino].pointers[pi] = p;
            inode[ino].size_allocated += 512;
        }
    }
    //printks("swr: %d\n", size_written);
    sdwrite((uint8_t *)inode, IARRAY_ADDR + ino_group*SECTOR_SIZE, SECTOR_SIZE);
    current_running->user_regs_context.regs[2] = (uint32_t)size_written;
}

void fseek(int fd, uint32_t offset)
{
    if(fd == -1)
        return;
    fd_array[fd].offset = offset;
}