/*
 *  #############################
 *  # zxfs (zero x file system) #
 *  #############################
 *
 *  util/dbzx/dbzx.c
 *
 *  Copyright (c) 2013 cg011235 <cg011235@gmail.com>
 *
 *  dbzx - debugger for zxfs file system
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include <endian.h>
#include "../../fs/zxfs.h"

#define TRUE    1

int
main(int argc, char *argv[])
{
    char cmd;
    struct stat buf;
    int devfd;
    zx_super_t sb;
    zx_inode_t in;
    zx_dirent_t dir;
    char ibitmap[65];
    char bbitmap[577];
    int i, j, p;
    time_t t;
    struct tm *tm;
    char blk[ZX_BLOCK_SIZE];
    char tbuf[1];

    /*
     * Check if correct arguments are passed
     */
    if (argc != 2) {
        printf("Usage: dbzx <block device>\n");
        exit(EXIT_FAILURE);
    }

    /*
     * Confirm that we can access given device file
     * and is a block device.
     */
    if (stat(argv[1], &buf) < 0) {
        fprintf(stderr, "mkzx: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    if (!S_ISBLK(buf.st_mode)) {
        fprintf(stderr, "mkzx: %s is not a block device\n", argv[1]);
        exit(EXIT_FAILURE);
    }
    printf("[OK]\tgiven device is block device\n");

    if ((devfd = open(argv[1], O_RDONLY)) < 0) {
        fprintf(stderr, "mkzx: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    printf("[OK]\topened devie\n");

    if (lseek(devfd, ZX_SEEK_BLOCKS * ZX_BLOCK_SIZE, SEEK_SET) < 0) {
        fprintf(stderr, "mkzx: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    printf("[OK]\tfound desirable space\n");

    printf("[OK]\tall set to launch dbzx (press h for help)\n");

    while (printf("dbzx:>") && scanf("%s", &cmd)) {
        switch (cmd) {
            case 'q' :
                exit(EXIT_SUCCESS);
                break;

            case 's' :
                if (lseek(devfd, ZX_SUPER_START * ZX_BLOCK_SIZE, SEEK_SET) < 0) {
                    fprintf(stderr, ": %s\n", strerror(errno));
                    break;
                }
                memset(&sb, 0, ZX_SUPER_SIZE);
                if (read(devfd, &sb, ZX_SUPER_SIZE) != ZX_SUPER_SIZE) {
                    fprintf(stderr, ": %s\n", strerror(errno));
                    break;
                }
                printf("\tmagic: %#x\n", le32toh(sb.s_magic));
                printf("\tstate: %#x\n", le32toh(sb.s_state));
                printf("\tfree inodes: %d\n", le32toh(sb.s_free_inodes));
                printf("\tfree blocks: %d\n", le32toh(sb.s_free_blocks));
                memset(ibitmap, 0, sizeof(ibitmap));
                for (i = 0; i < 64; i++) {
                    if (le64toh(sb.s_inode_list) & ((long)1 << (long)i)) {
                        ibitmap[i] = '1';
                    } else {
                        ibitmap[i] = '0';
                    }
                }
                ibitmap[64] ='\0';
                printf("\tinode bitmap: %s\n", ibitmap);

                memset(bbitmap, 0, sizeof(bbitmap));
                p = 0;
                for (i = 0; i < 18; i++) {
                    for (j = 0; j < 32; j++) {
                        if (sb.s_block_list[i] & (1 << j)) {
                            bbitmap[p++] = '1';
                        } else {
                            bbitmap[p++] = '0';
                        }
                    }
                }
                bbitmap[576] ='\0';
                printf("\tblock bitmap: %s\n", bbitmap);
                break;

            case 'i':
                printf("\tino:");
                scanf("%d", &i);

                if (lseek(devfd, ((ZX_INODE_START * ZX_BLOCK_SIZE) + (i * ZX_INODE_SIZE)), SEEK_SET) < 0) {
                    fprintf(stderr, ": %s\n", strerror(errno));
                    break;
                }
                memset(&in, 0, ZX_INODE_SIZE);
                if (read(devfd, &in, ZX_INODE_SIZE) != ZX_INODE_SIZE) {
                    fprintf(stderr, ": %s\n", strerror(errno));
                    break;
                }
                printf("\tmode: %#x\n", in.i_mode);
                printf("\tlinks: %hu\n", in.i_links);
                t = in.i_atime;
                tm = localtime (&t);
                printf("\tatime: %s", (char *) asctime(tm));
                t = in.i_mtime;
                tm = localtime (&t);
                printf("\tmtime: %s", (char *) asctime(tm));
                t = in.i_ctime;
                tm = localtime (&t);
                printf("\tctime: %s", (char *) asctime(tm));
                printf("\tuid: %hu\n", in.i_uid);
                printf("\tgid: %hu\n", in.i_gid);
                printf("\tsize: %hu\n", in.i_size);
                printf("\tblocks: %hu\n", in.i_blocks);
                printf("\tblock addr:\n");
                for (i = 0; i < le16toh(in.i_blocks); i++) {
                    printf("\t\t%d:%d\n", i + 1, le32toh(in.i_block[i]));
                }
                break;

            case 'd':
                printf("\tbno:");
                scanf("%d", &i);

                if (lseek(devfd, (i * ZX_BLOCK_SIZE), SEEK_SET) < 0) {
                    fprintf(stderr, ": %s\n", strerror(errno));
                    break;
                }

                for (j = 0; j < 16; j++) {
                    memset(&dir, 0, ZX_DIR_SIZE);
                    if (read(devfd, &dir, ZX_DIR_SIZE) != ZX_DIR_SIZE) {
                        fprintf(stderr, ": %s\n", strerror(errno));
                        break;
                    }
                    printf("\t%d:\t%u\t%s\n", j, dir.d_inode, dir.d_name);
                }
                break;

            case 'h':
                printf("dbzx command help\n");
                printf("\th: print help menu\n");
                printf("\tq: quit\n");
                printf("\ts: print super block\n");
                printf("\ti: print inode\n");
                printf("\td: print directory entries\n");
                printf("\tp: print disk block in hex\n");
                break;

            case 'p':
                printf("\tbno:");
                scanf("%d", &i);

                if (lseek(devfd, (i * ZX_BLOCK_SIZE), SEEK_SET) < 0) {
                    fprintf(stderr, ": %s\n", strerror(errno));
                    break;
                }
                memset(&blk, 0, ZX_BLOCK_SIZE);
                if (read(devfd, &blk, ZX_BLOCK_SIZE) != ZX_BLOCK_SIZE) {
                    fprintf(stderr, ": %s\n", strerror(errno));
                    break;
                }

                for (j = 1; j <= ZX_BLOCK_SIZE; j++) { 
                    printf("\t%#x", blk[j - 1]);
                    if (j % 9 == 0)
                        printf("\n");
                }
                printf("\n");
                break;
        }
    }

    return 0;
}
