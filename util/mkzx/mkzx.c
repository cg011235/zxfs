/*
 *  #############################
 *  # zxfs (zero x file system) #
 *  #############################
 *
 *  util/mkzx/mkzx.c
 *
 *  Copyright (c) 2013 cg011235 <cg011235@gmail.com>
 *
 *  mkzx - creat zxfs on disk layout on to given block device
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <endian.h>
#include <time.h>
#include "../../fs/zxfs.h"

int
main(int argc, char *argv[])
{
    zx_super_t sb;
    zx_inode_t ri;
    zx_dirent_t dir;
    char *dev;
    struct stat stbuf;
    int fd;

    /*
     * Check for correct arguments
     */ 
    if (argc != 2) {
        printf("\tUsage: mkzx <block_device>\n");
        return EXIT_FAILURE;
    }
    printf("[OK]\targuments verified\n");

    dev = argv[1];
    /*
     * Let's verify that it is indeed a block device
     */
    if (stat(dev, &stbuf) == -1) {
        fprintf(stderr, "[NOT OK] <%s> %s\n", dev, strerror(errno));
        exit(EXIT_FAILURE);
    }
    printf("[OK]\table to stat given device [%s]\n", dev);

    if (!S_ISBLK(stbuf.st_mode)) {
        fprintf(stderr, "[NOT OK] <%s> is not a block device\n", dev);
        exit(EXIT_FAILURE);
    }
    printf("[OK]\tconfirmed that device [%s] is a block device\n", dev);

    if ((fd = open(dev, O_WRONLY)) == -1) {
        fprintf(stderr, "[NOT OK] <%s> %s\n", dev, strerror(errno));
        exit(EXIT_FAILURE);
    }
    printf("[OK]\table to open given device [%s]\n", dev);

    /*
     * Make sure we have enough space on device 
     */
    if (lseek(fd, (ZX_BLOCK_SIZE * ZX_SEEK_BLOCKS), SEEK_SET) == (off_t) -1) {
        fprintf(stderr, "[NOT OK] <%s> %s\n", dev, strerror(errno));
        exit(EXIT_FAILURE);
    }
    printf("[OK]\tconfirmed that device [%s] has desired space\n", dev);

    /*
     * Fill super block structure and then write it on disk
     */
    memset(&sb, 0, ZX_SUPER_SIZE);
    sb.s_magic = htole32(ZX_MAGIC_NUMBER);
    sb.s_state = ZX_VALID_FS;
    sb.s_free_inodes = ZX_MAX_INODES - 1;
    sb.s_inode_list = 0;
    SET_BIT(sb.s_inode_list, 0);
    sb.s_free_blocks = ZX_MAX_BLOCKS - 1;
    SET_BIT(sb.s_block_list[0], 0);

    if (lseek(fd, (ZX_BLOCK_SIZE * ZX_SUPER_START), SEEK_SET) == (off_t) -1) {
        fprintf(stderr, "[NOT OK] <%s> %s\n", dev, strerror(errno));
        exit(EXIT_FAILURE);
    }

    if (write(fd, &sb, ZX_SUPER_SIZE) == -1) {
        fprintf(stderr, "[NOT OK] <%s> %s\n", dev, strerror(errno));
        exit(EXIT_FAILURE);
    }
    printf("[OK]\twrote super block on device [%s]\n", dev);

    /*
     * Fill inode structure for root directory and then 
     * write it on disk
     */
    memset(&ri, 0, ZX_INODE_SIZE);
    ri.i_mode = (S_IFDIR | S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
    ri.i_links = ZX_ROOT_INIT_LNKS;
    ri.i_atime = time(NULL);
    ri.i_mtime = time(NULL);
    ri.i_ctime = time(NULL);
    ri.i_uid = ZX_ROOT_UID;
    ri.i_gid = ZX_ROOT_GID;
    ri.i_size = ZX_BLOCK_SIZE;
    ri.i_blocks = ZX_ROOT_INIT_BLKS;
    ri.i_block[0] = ZX_DATA_START * ZX_BLOCK_SIZE;

    if (lseek(fd, (ZX_INODE_START * ZX_BLOCK_SIZE), SEEK_SET) == (off_t) -1) {
        fprintf(stderr, "[NOT OK] <%s> %s\n", dev, strerror(errno));
        exit(EXIT_FAILURE);
    }

    if (write(fd, &ri, ZX_INODE_SIZE) == -1) {
        fprintf(stderr, "[NOT OK] <%s> %s\n", dev, strerror(errno));
        exit(EXIT_FAILURE);
    }
    printf("[OK]\twrote root inode on device [%s]\n", dev);

    /*
     * Now we create both . and .. directory entries and make it
     * point to root inode
     */
    memset(&dir, 0, ZX_DIR_SIZE);
    dir.d_inode = ZX_ROOT_INODE;
    strcpy(dir.d_name, ".");

    if (lseek(fd, (ZX_DATA_START * ZX_BLOCK_SIZE), SEEK_SET) == (off_t) -1) {
        fprintf(stderr, "[NOT OK] <%s> %s\n", dev, strerror(errno));
        exit(EXIT_FAILURE);
    }

    if (write(fd, &dir, ZX_DIR_SIZE) == -1) {
        fprintf(stderr, "[NOT OK] <%s> %s\n", dev, strerror(errno));
        exit(EXIT_FAILURE);
    }

    memset(&dir, 0, ZX_DIR_SIZE);
    dir.d_inode = ZX_ROOT_INODE;
    strcpy(dir.d_name, "..");

    if (write(fd, &dir, ZX_DIR_SIZE) == -1) {
        fprintf(stderr, "[NOT OK] <%s> %s\n", dev, strerror(errno));
        exit(EXIT_FAILURE);
    }
    printf("[OK]\twrote directory entry for . & .. on device [%s]\n", dev);

    close(fd);
    
    printf("[OK]\tzxfs file system created on device [%s]\n", dev);
    printf("[OK]\tall done\n");

    return EXIT_SUCCESS;
}
