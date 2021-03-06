#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>

#define TIMER_T struct timeval
#define TIMER_READ(time) gettimeofday(&(time), NULL)
#define TIMER_DIFF_SECONDS(start, stop) \
    (((double)(stop.tv_sec)  + (double)(stop.tv_usec / 1000000.0)) - \
     ((double)(start.tv_sec) + (double)(start.tv_usec / 1000000.0)))

enum {ARRAY_SIZE = 1000,};

const unsigned long int LOOP_ITERATIONS = (1 << 25);

pthread_mutex_t m __attribute__((aligned(64))) = PTHREAD_MUTEX_INITIALIZER;
pthread_barrier_t b;
int global_array[ARRAY_SIZE];

void *reader(void *arg)
{
    int tmp = 0;
    volatile int old = 5;
    
    pthread_barrier_wait(&b);

    while(tmp != 6) {
        old = 5;
        __asm __volatile ( "lock; cmpxchgl %3, %0\n\t"            \
                           : "=m" (*(int *)&m)                    \
                           : "a" (old), "m" (*(int *)&m), "S" (7) \
                           : "cc", "memory");
        tmp = *(int *)&m;
    }
    
    return NULL;
}

int main(int argc, char **argv)
{
    int nthr = 1;
    pthread_t r_id;
    TIMER_T begin;
    TIMER_T end;

    if (argv[1][0] == 'y')
        nthr = atoi(argv[2]);

    pthread_barrier_init(&b, NULL, nthr);


    for (int i = 0; i < nthr - 1; i++) {
        pthread_create(&r_id, NULL, reader, NULL);
    }

    pthread_barrier_wait(&b);
    
    TIMER_READ(begin);
    for (int i = 0; i < LOOP_ITERATIONS; i++) {
        pthread_mutex_lock(&m);
        global_array[i % ARRAY_SIZE]++;
        pthread_mutex_unlock(&m);
    }
    TIMER_READ(end);

    printf("time %lf\n", TIMER_DIFF_SECONDS(begin, end));

    pthread_barrier_destroy(&b);

    return 0;
}
