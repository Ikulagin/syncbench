#ifndef SMART_MUTEX_H
#define SMART_MUTEX_H

#define __SMART_MUTEX_SIZE 8

#define SMART_MUTEX_INITIALIZER {{0, 0}}

typedef union {
    struct {
        int lock;
        int queue_length;
    } __data;
    char __size[__SMART_MUTEX_SIZE];
    long int __align;
} smart_mutex_t;

int smart_mutex_init(smart_mutex_t *m);
int smart_mutex_lock(smart_mutex_t *m, int *prof_array);
int smart_mutex_unlock(smart_mutex_t *m);

#endif //SMART_MUTEX_H
