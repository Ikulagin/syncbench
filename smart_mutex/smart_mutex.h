#ifndef SMART_MUTEX_H
#define SMART_MUTEX_H

#define __SMART_MUTEX_SIZE 4

#define SMART_MUTEX_INITIALIZER {{0}}

typedef union {
    struct {
        int lock;
    } __data;
    char __size[__SMART_MUTEX_SIZE];
    int __align;
} smart_mutex_t;

int smart_mutex_init(smart_mutex_t *m);
int smart_mutex_lock(smart_mutex_t *m);
int smart_mutex_unlock(smart_mutex_t *m);

#endif //SMART_MUTEX_H
