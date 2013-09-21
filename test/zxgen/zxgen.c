/*
 *  #############################
 *  # zxfs (zero x file system) #
 *  #############################
 *
 *  test/zxgen/zxgen.c
 *
 *  Copyright (c) 2013 cg011235 <cg011235@gmail.com>
 *
 *  zxgen - metadata and io load generator for zxfs
 *
 */

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define TRUE    1
#define FULL    1
#define EMPTY   2
#define LEN     31

/* Load policies */
#define LP_FIL_H    0
#define LP_DIR_V    1
#define LP_DIR_H    2
#define LP_DIR_F    3
#define LP_MIX_H    4
#define LP_MIX_V    5
#define LP_MIX_F    6

#define IOP_NO_VERIFY   0
#define IOP_DO_VERIFY   1  

struct iqe {
    char *path;
    struct iqe *next;
};

struct iq {
    int count;
    int policy;
    struct iqe *head;
    struct iqe *rear;
    pthread_cond_t wakeup_creator;
    pthread_cond_t wakeup_destroyer;
    pthread_mutex_t mlock;
} q = {0, 0, NULL, NULL, PTHREAD_COND_INITIALIZER, PTHREAD_COND_INITIALIZER, PTHREAD_MUTEX_INITIALIZER};

struct carg {
    int max_inode;
    int csleep;
};

struct fioarg {
    int maxblks;
    int blksize;
    int iopolicy;
};

void
zxlog(char *format, ...)
{
    time_t curtime;
    va_list args;
                 
    curtime = time (NULL);
    printf("%s\t", ctime(&curtime));
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
}

void
enq(char *path)
{
    struct iqe *e = NULL;

    e = (struct iqe *) malloc(sizeof(struct iqe));
    if (e) {
        e->path = (char *) malloc(LEN);
        strncpy(e->path, path, LEN - 1);
        e->path[LEN] = '\0';
        e->next = NULL;
        
        if (q.head == NULL) {
            q.head = q.rear = e;
        } else {
            q.rear->next = e;
            q.rear = e;
        }
        q.count = q.count + 1;
    }

    return;
}

void
deq(void)
{
    struct iqe *t;

    if (q.head != NULL) {
        t = q.head;
        if (t->next == NULL) {
            q.head = q.rear = NULL;
        } else {
            q.head = t->next;
        }
    }
    q.count = q.count - 1;
    free(t);

    return;
}

char *
getname(void)
{
    char *name = NULL;
    int len = (rand() % 30) + 1;
    int i;

    name = (char *) malloc(len + 1);
    if (name) {
        for (i = 0; i < len; i++) {
            if ((rand() % 10) == 0) {
                name[i] = (char) (65 + ((rand() + 1) % 10) + (i % 10));
            } else if ((rand() % 10) == 1) {
                name[i] = (char) (48 + ((rand() + 1) % 10));
            } else {
                name[i] = (char) (97 + ((rand() + 1) % 10) + (i % 10));
            }
        }
        name[len] = '\0';
    }

    return name;
}

void *
creator(void *arg)
{
    char *name;
    int i, fd;
    struct carg *c = (struct carg *) arg;
    
    while (TRUE) {
        name = getname();
        pthread_mutex_lock(&q.mlock);
        if (q.count < c->max_inode) {
            if (q.policy == LP_FIL_H) {
                if ((fd = creat(name, 0775)) == -1) {
                    fprintf(stderr, "Error: zxgen: %s, %s", name, strerror(errno));
                    continue;
                }
                zxlog("Created file %s[%d]\n", name, q.count);
                close(fd);
            }
            enq(name);
        } else {
            pthread_cond_wait(&q.wakeup_creator, &q.mlock);
        }
        pthread_mutex_unlock(&q.mlock);
        free(name);
        sleep(c->csleep);
    }

    pthread_exit(NULL);
}

void *
destroyer(void *arg)
{
    while (TRUE) {
        sleep(*(int *) arg);
        pthread_mutex_lock(&q.mlock);
        if (q.count > 0) {
            if (remove(q.head->path) == -1) {
                fprintf(stderr, "Error: zxgen: %s, %s", q.head->path, strerror(errno));
                continue;
            }
            zxlog("Destroyed file %s[%d]\n", q.head->path, q.count);
            deq();
        } else {
            pthread_cond_wait(&q.wakeup_destroyer, &q.mlock);
        }
        pthread_mutex_unlock(&q.mlock);
    }

    pthread_exit(NULL);
}

void *
fileio(void *arg)
{
    struct iqe *t = NULL;
    struct fioarg *f = (struct fioarg *) arg;
    int i, j, fd;
    char *block = (char *) malloc(f->blksize);
    char *block2 = (char *) malloc(f->blksize);

    while (TRUE) {
        pthread_mutex_lock(&q.mlock);
        for (t = q.head; t != NULL; t = t->next) {
            if ((fd = open(t->path, O_RDWR)) == -1) {
                fprintf(stderr, "Error: zxgen: %s, %s", t->path, strerror(errno));
                continue;
            }
            for (i = 0; i < f->maxblks; i++) {
                if (((rand() % 10) + 1) % ((rand() % 10) + 1) == 0) {
                    continue;
                }
                for (j = 0; j < f->blksize; j++) {
                    block[i] = (char) (rand() % 10);
                }
                if (pwrite(fd, block, f->blksize, (i * f->blksize)) == -1) {
                    fprintf(stderr, "Error: zxgen: %s, %s", t->path, strerror(errno));
                    continue;
                }
                zxlog("Wrote block %d of %s\n", i, t->path);
                if (f->iopolicy == IOP_DO_VERIFY) {
                    if (pread(fd, block2, f->blksize, (i * f->blksize)) == -1) {
                        fprintf(stderr, "Error: zxgen: %s, %s", t->path, strerror(errno));
                        continue;
                    }
                    zxlog("Read block %d of %s\n", i, t->path);
                    if (strcmp(block, block2) != 0) {
                        fprintf(stderr, "Error: zxgen: %s, %s", t->path, strerror(errno));
                        continue;
                    }
                    zxlog("Verified block %d of %s\n", i, t->path);
                }
            }
            if ((rand() % 10) < 2) {
                if (truncate(t->path, (rand() % 10)) == -1) {
                    fprintf(stderr, "Error: zxgen: %s, %s", t->path, strerror(errno));
                    continue;
                }
                zxlog("Truncated %s\n", t->path);
            }
            close(fd);
        }
        pthread_mutex_unlock(&q.mlock);
    }
    free(block);
    free(block2);

    pthread_exit(NULL);
}

int
main(int argc, char *argv[])
{
    int opt;
    char *mount_point = NULL;
    struct carg carg = {63, 2};
    struct fioarg f = {9, 512, IOP_NO_VERIFY};
    int dsleep = 3;
    pthread_t ptc1, ptc2, ptc3;
    pthread_t ptd1, ptd2;
    pthread_t ptf;

    while ((opt = getopt(argc, argv, "m:i:c:d:")) != -1) {
        switch (opt) {
            case 'm':
                mount_point = (char *) optarg;
                break;
            case 'i':
                carg.max_inode = atoi(optarg);
                break;
            case 'c':
                carg.csleep = atoi(optarg);
                break;
            case 'd':
                dsleep = atoi(optarg);
                break;
            case 'b':
                f.blksize = atoi(optarg);
                break;
            case 'k':
                f.maxblks = atoi(optarg);
                break;
            case 'p':
                f.iopolicy = atoi(optarg);
                break;
            case '?':
            default:
                fprintf(stderr, "Usage:\n\tzxgen -m <mount point> [-i <max inodes>]"
                                " [-c <creator sleep>] [-d <destroyer sleep>]"
                                " [-b <block size>] [-k <max blocks>]"
                                " [-p <io policy>] [-l <log file>]");
                exit(EXIT_FAILURE);
        }
    }

    if ((optind == 1 || optind < argc) || mount_point == NULL) {
        fprintf(stderr, "Usage:\n\tzxgen -m <mount point> [-i <max inodes>]"
                        " [-c <creator sleep>] [-d <destroyer sleep>]"
                        " [-b <block size>] [-k <max blocks>]"
                        " [-p <io policy>] [-l <log file>]");
        exit(EXIT_FAILURE);
    }

    if ((mount_point != NULL) && (chdir(mount_point) == -1)) {
        fprintf(stderr, "Error: zxgen: %s: %s\n", mount_point, strerror(errno));
        exit(EXIT_FAILURE);
    }

    zxlog("Working directory changed to %s\n", mount_point);

    pthread_create(&ptc1, NULL, creator, (void *) &carg);
    pthread_create(&ptc2, NULL, creator, (void *) &carg);
    pthread_create(&ptc3, NULL, creator, (void *) &carg);
    pthread_create(&ptf, NULL, fileio, (void *) &f);
    pthread_create(&ptd1, NULL, destroyer, (void *) &dsleep);
    pthread_create(&ptd2, NULL, destroyer, (void *) &dsleep);

    pthread_exit(NULL);
}
