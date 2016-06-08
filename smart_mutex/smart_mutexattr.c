#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "smart_mutex.h"
#include "internaltypes.h"

//#define SMART_MUTEXATTR_MASK 

int smart_mutexattr_init(smart_mutexattr_t *a)
{
    if (sizeof(struct smart_mutexattr) != sizeof(smart_mutexattr_t))
        memset(a, '\0', sizeof(*a));

    ((struct smart_mutexattr *)a)->mutexkind = PTHREAD_MUTEX_NORMAL;

    return 0;
}

int smart_mutexattr_settype(smart_mutexattr_t *a, int kind)
{
    struct smart_mutexattr *tmpattr = NULL;

    if (kind < SMART_MUTEX_NORMAL || kind > SMART_MUTEX_CONTENTION_PROF)
        return -1;

    tmpattr = (struct smart_mutexattr *) a;

    tmpattr->mutexkind =  kind;
    
    return 0;
}

int smart_mutexattr_setcontentionstats(smart_mutexattr_t *a,
                                       int **contention_stats)
{
    struct smart_mutexattr *tmpattr = NULL;
    
    if (contention_stats == NULL)
        return -1;

    tmpattr = (struct smart_mutexattr *) a;

    tmpattr->contention_stats = contention_stats;
    
    return 0;
}
