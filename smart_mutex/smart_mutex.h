#ifndef SMART_MUTEX_H
#define SMART_MUTEX_H

#ifdef __x86_64__
#  define __SMART_MUTEX_SIZE 24
#  define __SMART_MUTEXATTR_SIZE 12
# else
#  define __SMART_MUTEX_SIZE 16
#  define __SMART_MUTEXATTR_SIZE 8
#endif

#define SMART_MUTEX_INITIALIZER {{0, 0, 0}}

enum {
    SMART_MUTEX_NORMAL = 0,
    SMART_MUTEX_CONTENTION_PROF,
};

typedef union {
    struct {
        int lock;
        int queue_length;
        int kind;
        int *contention_stats;
    } __data;
    char __size[__SMART_MUTEX_SIZE];
    long int __align;
} smart_mutex_t;


typedef union {
    char __size[__SMART_MUTEXATTR_SIZE];
    int __align;
} smart_mutexattr_t;

extern int smart_mutex_init(smart_mutex_t *m, smart_mutexattr_t *a);
extern int smart_mutex_lock(smart_mutex_t *m);
extern int smart_mutex_unlock(smart_mutex_t *m);

extern int smart_mutexattr_init(smart_mutexattr_t *a);
extern int smart_mutexattr_settype(smart_mutexattr_t *a, int kind);
extern int smart_mutexattr_setcontentionstats(smart_mutexattr_t *a,
                                              int *contention_stats);

#endif //SMART_MUTEX_H
