#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "smart_mutex.h"

#include "lowlevellock.h"

#define LLL_MUTEX_LOCK(mutex) \
    lll_lock((mutex)->__data.lock)

#define LLL_MUTEX_TRY_LOCK(mutex) \
    lll_trylock((mutex)->__data.lock)

#define LLL_MUTEX_UNLOCK(mutex) \
    lll_unlock((mutex)->__data.lock)

#define atomic_spin_nop() asm ("rep; nop")

int smart_mutex_init(smart_mutex_t *m)
{
    memset(m, 0, __SMART_MUTEX_SIZE);
    return 0;
}


int smart_mutex_lock(smart_mutex_t *m)
{
    assert(sizeof(m->__size) == sizeof(m->__data));
    
    int cnt = 0;
    int max_cnt = 512;

    if (LLL_MUTEX_TRY_LOCK(m) != 0) {
        do {
            if (cnt++ >= max_cnt) {
                LLL_MUTEX_LOCK(m);
                break;
            }
            atomic_spin_nop();
        } while (LLL_MUTEX_TRY_LOCK(m) != 0);
    }

    return 0;
}

int smart_mutex_unlock(smart_mutex_t *m)
{
    LLL_MUTEX_UNLOCK(m);

    return 0;
}