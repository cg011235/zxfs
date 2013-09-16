/*
 *  ############################
 *  # zxfs (zero x file system #
 *  ############################
 *   
 *  fs/zxfs.h
 * 
 *  Copyright (c) 2013 cg011235 <cg011235@gmail.com>
 *
 *  Declaration of zxfs structures, macros and globals goes here.
 *
 *  On disk layout of zxfs
 *    ___________________________________________________________________________________
 *   |                   |                   |                                           |
 *   | super block       |  inode block      |  Data block                               |
 *   |                   |                   |                                           |
 *   |                   |                   |                                           |
 *   |                   |                   |                                           |
 *   |___________________|___________________|___________________________________________|
 *       /           \     /                  \
 *      /             \   /                    \
 *      ---------------   ___________________________________________________________
 *     | s_magic       | |         |         |         |       |                     |
 *     | s_state       | | inode 1 | inode 2 | inode 3 | ...   | inode ZX_MAX_INODES |
 *     | s_free_inodes | |_________|_________|_________|_______|_____________________|
 *     | s_inode_list[]|                     /     \
 *     | s_free_blocks |                    /       \
 *     | s_block_list[]|                   /         \
 *      ---------------                    -----------  
 *      zx_super_t                        | i_mode    |   
 *                                        | i_atime   |
 *                                        | i_mtime   |
 *                                        | i_ctime   |
 *                                        | i_uid     |
 *                                        | i_gid     |   
 *                                        | i_size    |   
 *                                        | i_blocks  |
 *                                        | i_block[] |
 *                                        | i_pad[]   |
 *                                         -----------
 *                                         zx_inode_t
 *
 */


#ifndef ZXFS_H
#define ZXFS_H

#include <linux/fs.h>
#include <linux/types.h>
#include <linux/kernel.h>

#define ZX_BLOCK_SIZE       512
#define ZX_MAGIC_NUMBER     0x2E40f500 
#define ZX_MAX_INODES       64
#define ZX_MAX_LINKS        3
#define ZX_MAX_BLOCKS       576
#define ZX_DIRECT_BLOCKS    9
#define ZX_BLOCK_LIST_SIZE  18
#define ZX_INODE_PADDING    2
#define ZX_SKIP_BLOCKS      4
#define ZX_SUPER_START      ZX_SKIP_BLOCKS
#define ZX_INODE_START      ZX_SUPER_START + 1
#define ZX_DATA_START       14
#define ZX_VALID_FS         0
#define ZX_ERROR_FS         1
#define ZX_ROOT_UID         0
#define ZX_ROOT_GID         0
#define ZX_ROOT_INODE       0
#define ZX_ROOT_INIT_BLKS   1
#define ZX_ROOT_INIT_LNKS   2

/*
 * The zxfs superblock : 128 byte
 */
typedef struct zx_super_block {                 /*  1024    bits */
    __le32   s_magic;
    __le32   s_state;
    __le32   s_free_inodes;
    __le64   s_inode_list;                       /*  64      bits */
    __le32   s_free_blocks;
    __le32   s_block_list[ZX_BLOCK_LIST_SIZE];   /*  576     bits */
} zx_super_t;

#define ZX_SUPER_SIZE   sizeof(zx_super_t)
#define ZX_SUPER_BLOCKS (ZX_SUPER_SIZE % ZX_BLOCK_SIZE)

typedef struct zx_super_inmem {
    struct buffer_head *sbh;
    zx_super_t * sb;
    /*
     * Lock to protect concurrent access to sb.
     * Concurrent access will be when same block
     * device having zxfs is mounted on multiple
     * mount points.
     */
#ifdef KERNEL
    spinlock_t s_lock;
#endif
} zx_super_inmem_t;
/*
 *  The zxfs inode : 64 byte
 */
typedef struct zx_inode {               /* |links|FileTyp|user |group|world| */
    __le16   i_mode;                     /* [ | | | | | | |r|w|x|r|w|x|r|w|x] */
    __le16   i_links;
    __le32   i_atime;
    __le32   i_mtime;
    __le32   i_ctime;
    __le16   i_uid;
    __le16   i_gid;
    __le16   i_size;
    __le16   i_blocks;
    __le32   i_block[ZX_DIRECT_BLOCKS];
    __le16   i_pad[ZX_INODE_PADDING];
} zx_inode_t;

#define ZX_INODE_SIZE   sizeof(zx_inode_t)
#define ZX_INODE_LIST_BLOCKS ((ZX_MAX_INODES * ZX_INODE_SIZE) / ZX_BLOCK_SIZE)

#define ZX_INODE_PER_BLOCK (ZX_BLOCK_SIZE / ZX_INODE_SIZE)
#define ZX_SEEK_BLOCKS  (ZX_MAX_BLOCKS + ZX_SUPER_BLOCKS + ZX_INODE_LIST_BLOCKS +ZX_SKIP_BLOCKS + 1)

#define SET_BIT(bits, pos) bits ^= ((long) 1 << pos)
#define IFSET(bits, pos)   (bits & (1 << pos))

#define SET_WORLD_X(mode)  SET_BIT(mode, 0)
#define SET_WORLD_W(mode)  SET_BIT(mode, 1)
#define SET_WORLD_R(mode)  SET_BIT(mode, 2)
#define SET_GROUP_X(mode)  SET_BIT(mode, 3)
#define SET_GROUP_W(mode)  SET_BIT(mode, 4)
#define SET_GROUP_R(mode)  SET_BIT(mode, 5)
#define SET_USER_X(mode)  SET_BIT(mode, 6)
#define SET_USER_W(mode)  SET_BIT(mode, 7)
#define SET_USER_R(mode)  SET_BIT(mode, 8)

#define ZX_MAX_NAME         31
/*
 * Directory entry
 */
typedef struct zx_dirent {
    __u8    d_inode;
    char    d_name[ZX_MAX_NAME];
} zx_dirent_t;

#define ZX_DIR_SIZE sizeof(zx_dirent_t)


extern struct address_space_operations  zx_aops;
extern struct inode_operations  zx_file_iops;
extern struct inode_operations  zx_dir_iops;
extern struct file_operations   zx_file_fops;
extern struct file_operations   zx_dir_fops;

extern struct inode * zx_iget(struct super_block *sb, unsigned long ino);

#endif /* ZXFS_H */
