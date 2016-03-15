#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "split_mutex.h"

#include "lowlevellock.h"

#define LLL_MUTEX_LOCK(mutex) \
    lll_lock((mutex)->__data.lock)

#define LLL_MUTEX_UNLOCK(mutex) \
    lll_unlock((mutex)->__data.lock)

int split_mutex_init(split_mutex_t *m)
{
    memset(m, 0, __SPLIT_MUTEX_SIZE);
    return 0;
}


int split_mutex_lock(split_mutex_t *m)
{
    assert(sizeof(m->__size) == sizeof(m->__data));
    

    LLL_MUTEX_LOCK(m);

    return 0;
}

int split_mutex_unlock(split_mutex_t *m)
{
    assert(sizeof(m->__size) == sizeof(m->__data));
    
    LLL_MUTEX_UNLOCK(m);

    return 0;
}
