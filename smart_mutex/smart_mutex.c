#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "smart_mutex.h"
#include "internaltypes.h"
#include "lowlevellock.h"


#define SMART_MUTEX_TYPE(m) \
    ((m)->__data.kind & 0x7f)

#define LLL_MUTEX_LOCK(mutex)                  \
    lll_lock((mutex)->__data.lock)

#define LLL_MUTEX_TRY_LOCK(mutex) \
    lll_trylock((mutex)->__data.lock)

#define LLL_CONTENTION_PROF(mutex)                  \
    lll_contprof((mutex)->__data.lock, \
                        (mutex)->__data.contention_stats)

#define LLL_MUTEX_UNLOCK(mutex) \
    lll_unlock((mutex)->__data.lock)

#define atomic_spin_nop() asm ("rep; nop")

int smart_mutex_init(smart_mutex_t *m, smart_mutexattr_t *a)
{
    const struct smart_mutexattr *tmpattr = (struct smart_mutexattr *) a;
    
    memset(m, 0, __SMART_MUTEX_SIZE);

    switch(tmpattr->mutexkind) {
    case SMART_MUTEX_CONTENTION_PROF:
        if (tmpattr->contention_stats == NULL)
            return -1;
        m->__data.contention_stats = tmpattr->contention_stats;
        break;
    case SMART_MUTEX_NORMAL:
        m->__data.kind = tmpattr->mutexkind;
        break;
    }
    return 0;
}


int smart_mutex_lock(smart_mutex_t *m)
{
    assert(sizeof(m->__size) == sizeof(m->__data));
    
    int type = SMART_MUTEX_TYPE(m);

    if (type == SMART_MUTEX_NORMAL) {
        LLL_MUTEX_LOCK(m);
    } else if (type == SMART_MUTEX_CONTENTION_PROF) {
        int indx = __sync_fetch_and_add(&(m->__data.queue_length), 1);
        __sync_fetch_and_add(&(m->__data.contention_stats[indx]), 1);
        LLL_CONTENTION_PROF(m);
    }
    
    /* int cnt = 0; */
    /* int max_cnt = 512; */

    /* if (LLL_MUTEX_TRY_LOCK(m) != 0) { */
    /*     do { */
    /*         if (cnt++ >= max_cnt) { */
    /*             break; */
    /*         } */
    /*         atomic_spin_nop(); */
    /*     } while (LLL_MUTEX_TRY_LOCK(m) != 0); */
    /* } */

    return 0;
}

int smart_mutex_unlock(smart_mutex_t *m)
{
    __sync_fetch_and_sub(&m->__data.queue_length, 1);
    LLL_MUTEX_UNLOCK(m);

    return 0;
}
