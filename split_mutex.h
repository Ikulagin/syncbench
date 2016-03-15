#ifndef SPLIT_MUTEX_H
#define SPLIT_MUTEX_H

#define __SPLIT_MUTEX_SIZE 4

#define SPLIT_MUTEX_INITIALIZER {{0}}

typedef union {
    struct {
        int lock;
    } __data;
    char __size[__SPLIT_MUTEX_SIZE];
    int __align;
} split_mutex_t;

int split_mutex_init(split_mutex_t *m);
int split_mutex_lock(split_mutex_t *m);
int split_mutex_unlock(split_mutex_t *m);

#endif //SPLIT_MUTEX_H
