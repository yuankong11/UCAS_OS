#ifndef INCLUDE_FS_H_
#define INCLUDE_FS_H_

#include "type.h"
#include "slist.h"

/*
 * super_block | imap | smap | iarray | reserved | darray
 * super_block: 512B
 * unit: sector, 512B
 * inode size: 64B
 * iarray: 4K items, 256KB
 * data block size: 512B
 * darray: 1M items, 512MB
 * imap: 4K bits, 512B
 * smap: 1M bits, 128KB
 * reserved: 127KB
 */

#define MAGIC_NUMBER 0x99569A78

#define SECTOR_SIZE 0x200

#define FS_BASE     0x01000000
#define IMAP_ADDR   0x01000200 // inode map
#define SMAP_ADDR   0x01000400 // sector map
#define IARRAY_ADDR 0x01020400 // inode array
#define DARRAY_ADDR 0x01080000 // data array

#define INODE_ITEM_NUM 4096
#define DATA_ITEM_NUM  1048576

#define FILE_NORMAL 1
#define FILE_DIR    2
#define FILE_SLINK  3

/*
 * bit map to sector
 * bit map: 7 6 5 4 3 2 1 0
 * sector : 7 6 5 4 3 2 1 0
 * the x-th bit to x-th sector
 */

typedef struct super_block
{
    // 512B
    uint32_t imap_offset;    // sectors
    uint32_t smap_offset;    // sectors
    uint32_t iarray_offset;  // sectors
    uint32_t darray_offset;  // sectors
    uint32_t inode_size;     // 64B
    uint32_t directory_size; // 512B
    uint32_t inode_number;   // 4K
    uint32_t data_number;    // 1M
    uint32_t data_used;      // used data items
    uint32_t inode_used;     // used inode items
    uint32_t reserved[118];
} super_block_t;

typedef struct inode
{
    // 64B
    // todo: size overflow
    uint8_t type;
    uint8_t links;
    uint8_t reserved[2];
    uint32_t size; // bytes
    uint32_t size_allocated; // bytes
    // 10 direct pointers
    // 1 first level indirect pointer
    // 1 second level indirect pointer
    // 1 third level indirect pointer
    // supported file size: 5KB + 64KB + 8MB + 1GB
    uint32_t pointers[13];
} inode_t;

#define MAX_NAME_LENTH   26
#define MAX_DIR_ITEM_NUM 15

typedef struct dmap
{
    // 1B + 27B + 4B = 32B
    uint8_t valid;
    char name[MAX_NAME_LENTH + 1];
    uint32_t ino;
} dmap_t;

typedef struct dir
{
    // directory
    // 4 + 27 + 1 + 15*32 = 512B
    uint32_t fpointer; // pointer to father dir
    char name[MAX_NAME_LENTH + 1];
    uint8_t reserved;
    dmap_t map[MAX_DIR_ITEM_NUM];
} dir_t;

typedef struct slink
{
    // 512B
    char path[512];
} slink_t;

#define O_RDWR 0 // read & write
#define O_RD   1 // read only
#define O_WR   2 // write only

typedef struct fd
{
    uint32_t ino;
    uint32_t access;
    uint32_t size;   // bytes
    uint32_t offset; // bytes
} fd_t;

#define FD_ARRAY_SIZE 64
#define FD_ARRAY_SLIST_SIZE FD_ARRAY_SIZE + 1
extern fd_t fd_array[FD_ARRAY_SIZE];
extern node_t fd_array_slist[FD_ARRAY_SLIST_SIZE];

void mkfs();
void mount();
void statfs();
void mkdir(char *s);
void rmdir(char *s);
void ls();
void cd(char *s);
void get_curr_dir_name();
void touch(char *s);

void fopen(char *s, int access);
void fclose(int fd);
void fread(int fd, uint8_t *buf, uint32_t size);
void fwrite(int fd, uint8_t *buf, uint32_t size);
void fseek(int fd, uint32_t offset);

#endif