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

struct iqe {
    char *path;
    struct iqe *next;
};

struct iq {
    int count;
    int policy;
    struct iqe *head;
    struct iqe *rear;
    pthread_cond_t wakeup_prod;
    pthread_mutex_t mlock;
} q = {0, -1, NULL, NULL, PTHREAD_COND_INITIALIZER, PTHREAD_MUTEX_INITIALIZER};


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



void *
producer(void *arg)
{
    char name[LEN];
    int i, fd;
    int maxi = *((int *) arg);
    
    while (TRUE) {
        srand(time(NULL));
        for (i = 0; i < (LEN - 1); i++) {
            if (i % 3 == 0) {
                name[i] = (char) (65 + (rand() % 10) + (i % 10)); 
            } else {
                name[i] = (char) (97 + (rand() % 10) + (i % 10)); 
            }
        }
        name[LEN - 1] = '\0';
        pthread_mutex_lock(&q.mlock);
        if (q.count < maxi) {
            if (q.policy == LP_FIL_H) {
                if ((fd = creat(name, 0775)) == -1) {
                    fprintf(stderr, "zxgen: %s, %s", name, strerror(errno));
                    continue;
                }
                close(fd);
            }
            enq(name);
        } else {
            pthread_cond_wait(&q.wakeup_prod, &q.mlock);
        }
        pthread_mutex_unlock(&q.mlock);
    }

    pthread_exit(NULL);
}

int
main(int argc, char *argv[])
{
    int opt;
    char *mount_point = NULL;
    int max_inode = 64;
    pthread_t prod;

    while ((opt = getopt(argc, argv, "m:i:")) != -1) {
        switch (opt) {
            case 'm':
                mount_point = (char *) optarg;
                break;
            case 'i':
                max_inode = atoi(optarg);
                break;
            case '?':
            default:
                fprintf(stderr, "Usage:\n\tzxgen -m <mount_point> -i <max inodes>\n");
                exit(EXIT_FAILURE);
        }
    }

    if (optind == 1 || optind < argc) {
        fprintf(stderr, "Usage:\n\tzxgen -m <mount_point> -i <max inodes>\n");
        exit(EXIT_FAILURE);
    }

    if ((mount_point != NULL) && (chdir(mount_point) == -1)) {
        fprintf(stderr, "zxgen: %s: %s\n", mount_point, strerror(errno));
        exit(EXIT_FAILURE);
    }

    pthread_create(&prod, NULL, producer, (void *) &max_inode);

    pthread_exit(NULL);
}
