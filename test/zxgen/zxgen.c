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

struct iqe {
    char *path;
    int flag;
    struct iqe *next;
};

struct iq {
    int count;
    struct iqe *head;
    struct iqe *rear;
    pthread_mutex_t mlock;
} q = {0, NULL, NULL, PTHREAD_MUTEX_INITIALIZER};

void
enq(char *path, int flag)
{
    struct iqe *e = NULL;
    int len = strlen(path);

    e = (struct iqe *) malloc(sizeof(struct iqe));
    if (e) {
        e->path = (char *) malloc(len + 1);
        strncpy(e->path, path, len);
        e->path[len] = '\0';
        e->flag = flag;
        e->next = NULL;
        
        pthread_mutex_lock(&q.mlock);
        if (q.head == NULL) {
            q.head = q.rear = e;
        } else {
            q.rear->next = e;
            q.rear = e;
        }
        q.count = q.count + 1;
        pthread_mutex_unlock(&q.mlock);
    }

    return;
}

void
deq(void)
{
    struct iqe *t;

    pthread_mutex_lock(&q.mlock);
    if (q.head != NULL) {
        t = q.head;
        if (t->next == NULL) {
            q.head = q.rear = NULL;
        } else {
            q.head = t->next;
        }
    }
    q.count = q.count - 1;
    pthread_mutex_unlock(&q.mlock);
    free(t);

    return;
}



void *
producer(void *arg)
{
    char name[7];
    
    srand(time(NULL));
    name[0] = 't';
    name[1] = (char) (65 + (rand() % 9)); 
    name[2] = (char) (97 + (rand() % 2)); 
    name[3] = (char) (65 + (rand() % 3)); 
    name[4] = (char) (97 + (rand() % 4)); 
    name[5] = (char) (48 + (rand() % 5)); 
    name[6] = '\0';
    mkdir(name, 0755);
    enq(name, 1);
    sleep(5);

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

    pthread_create(&prod, NULL, producer, NULL);

    pthread_exit(NULL);
}
