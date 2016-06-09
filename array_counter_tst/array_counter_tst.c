#define _GNU_SOURCE 
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <inttypes.h>
#include <string.h>
#include <pthread.h>
#include <sched.h>
#include <stdatomic.h>
#include <getopt.h>

#include "smart_mutex.h"
#include "hpctimer.h"

#define TIMER_T struct timeval
#define TIMER_READ(time) gettimeofday(&(time), NULL)
#define TIMER_DIFF_SECONDS(start, stop) \
    (((double)(stop.tv_sec)  + (double)(stop.tv_usec / 1000000.0)) - \
     ((double)(start.tv_sec) + (double)(start.tv_usec / 1000000.0)))  

#define RDTSC(Var)                                              \
    ({ unsigned int _hi, _lo;                                   \
        asm volatile ("rdtsc" : "=a" (_lo), "=d" (_hi));        \
        (Var) = ((unsigned long long int) _hi << 32) | _lo; })

enum {
    STR_LNGTH = 32,
    ARRAY_SIZE = 1024,
    NUM_ITERATION = 102400,
};

enum cs_method {
    NORMAL_PTHREAD_MTX = 0,
    ADAPTIVE_PTHREAD_MTX,
    PTHREAD_SPIN,
    SMART_MTX,
    STM,
    CS_METHOD_CNT
};

typedef struct global_params_s {
    int n_threads;
    int affinity;
    double (*cs_ptr)(int);
} global_params;

double cs_normal_pthread_mutex(int rank);
double cs_adaptive_pthread_mutex(int rank);
double cs_pthread_spin(int rank);
double cs_smart_mutex(int rank);
double cs_stm(int rank);

static char cs_method_pnames[CS_METHOD_CNT][STR_LNGTH] =
{ "normal-pthread-mtx",
  "adaptive-pthread-mtx",
  "pthread-spin",
  "smart-mtx",
  "stm" };
double (*cs_methods_array[CS_METHOD_CNT])(int) = 
{ cs_normal_pthread_mutex,
  cs_adaptive_pthread_mutex,
  cs_pthread_spin,
  cs_smart_mutex,
  cs_stm };
static global_params g_params;

pid_t gettid()
{
    return syscall(SYS_gettid);
}

pthread_mutex_t m __attribute__((aligned (64))) = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t m_adapt __attribute__((aligned (64))) = PTHREAD_ADAPTIVE_MUTEX_INITIALIZER_NP;
pthread_spinlock_t spin __attribute__((aligned (64)));
smart_mutex_t m_smart __attribute__((aligned (64))) = SMART_MUTEX_INITIALIZER;
int *prof_array __attribute__((aligned (64))) = NULL;
int global_array[ARRAY_SIZE]  __attribute__((aligned (64)));

/*The the critical section being measured with normal pthread_mutex*/
double cs_normal_pthread_mutex(int rank)
{
    double t_total = 0;

    for (int i = 0; i < NUM_ITERATION; i++) {
        double t = 0;

        t = hpctimer_wtime();
        pthread_mutex_lock(&m);

        global_array[i % ARRAY_SIZE]++;

        pthread_mutex_unlock(&m);
        t_total += hpctimer_wtime() - t;
    }

    return t_total;
}

/*The the critical section being measured with pthread_mutex*/
double cs_adaptive_pthread_mutex(int rank)
{
    double t_total = 0;
    
    for (int i = 0; i < NUM_ITERATION; i++) {
        double t = 0;

        t = hpctimer_wtime();
        pthread_mutex_lock(&m_adapt);

        global_array[i % ARRAY_SIZE]++;

        pthread_mutex_unlock(&m_adapt);
        t_total += hpctimer_wtime() - t;
    }

    return t_total;
}

/*The the critical section being measured with pthread_spin*/
double cs_pthread_spin(int rank)
{
    double t_total = 0;
    
    for (int i = 0; i < NUM_ITERATION; i++) {
        double t = 0;

        t = hpctimer_wtime();
        pthread_spin_lock(&spin);
        
        global_array[i % ARRAY_SIZE]++;

        pthread_spin_unlock(&spin);
        t_total += (hpctimer_wtime() - t);
    }

    return t_total;
}

/*The the critical section being measured with smart_mutex*/
double cs_smart_mutex(int rank)
{
    double t_total = 0;
    
    for (int i = 0; i < NUM_ITERATION; i++) {
        double t = 0;

        t = hpctimer_wtime();
        smart_mutex_lock(&m_smart);

        global_array[i % ARRAY_SIZE]++;

        smart_mutex_unlock(&m_smart);
        t_total += (hpctimer_wtime() - t);
    }

    return t_total;
}

/*The the critical section being measured with transactional memory*/
double cs_stm(int rank)
{
    double t_total = 0;
    
    for (int i = 0; i < NUM_ITERATION; i++) {
        double t = 0;

        t = hpctimer_wtime();
        __transaction_atomic {

            global_array[i % ARRAY_SIZE]++;

        }
        t_total += (hpctimer_wtime() - t);
    }

    return t_total;
}


typedef struct thread_info_s  {
    int rank;
    pid_t pid;
    pid_t tid;
    char status_path[ARRAY_SIZE];
} thread_info;

void ctxt_switch_get(thread_info *t, uint64_t *vltr, uint64_t *invltr, uint64_t *total)
{
    char buf[ARRAY_SIZE];
    FILE *f;
    
    if ((f = fopen(t->status_path, "r")) == NULL) {
        fprintf(stderr, "[%s:%d] fopen error\n", __func__, __LINE__);
        exit(EXIT_FAILURE);
    }

    while(!feof(f)) {
        if (fgets(buf, sizeof(buf), f) == NULL)
            break;
        if (!strncmp(buf, "voluntary_ctxt_switches:", 24)) {
            (void)sscanf(buf + 24, "%" SCNu64, vltr);
            continue;
        }
        if (!strncmp(buf, "nonvoluntary_ctxt_switches:", 27)) {
            (void)sscanf(buf + 27, "%" SCNu64, invltr);
            continue;
        }
    }
    fclose(f);
    *total = *vltr + *invltr;
}

typedef struct measured_params_s {
    double lock_time;
    uint64_t ctxt_sw_v; /*voluntary context switches*/
    uint64_t ctxt_sw_nonv; /*nonvoluntary context switches*/
    uint64_t ctxt_sw_total; /*voluntary and nonvoluntary context switches*/
} measured_params;

measured_params *m_params;
thread_info *t_info;

pthread_barrier_t barrier;
void *thread_fnc(void *arg)
{
    int rank = *(int *)arg;
    double t = 0;
    uint64_t ctxt_sw_v_before = 0, ctxt_sw_v_after = 0;
    uint64_t ctxt_sw_inv_before = 0, ctxt_sw_inv_after = 0;
    uint64_t ctxt_sw_total_before = 0, ctxt_sw_total_after = 0;

    t_info[rank].pid = getpid();
    t_info[rank].tid = gettid();
    sprintf(t_info[rank].status_path, "/proc/%d/task/%d/status", t_info[rank].pid, t_info[rank].tid);
    
    pthread_barrier_wait(&barrier);

    ctxt_switch_get(&t_info[rank], &ctxt_sw_v_before, &ctxt_sw_inv_before, &ctxt_sw_total_before);
    t = g_params.cs_ptr(rank);
    ctxt_switch_get(&t_info[rank], &ctxt_sw_v_after, &ctxt_sw_inv_after, &ctxt_sw_total_after);

    pthread_barrier_wait(&barrier);

    m_params[rank].lock_time = t / NUM_ITERATION;
    m_params[rank].ctxt_sw_v = ctxt_sw_v_after - ctxt_sw_v_before;
    m_params[rank].ctxt_sw_nonv = ctxt_sw_inv_after - ctxt_sw_inv_before;
    m_params[rank].ctxt_sw_total = ctxt_sw_total_after - ctxt_sw_total_before;

    return NULL;
}

void print_measured_params()
{
    double avg_lock_time = 0;
    uint64_t avg_ctxt_sw_v = 0, avg_ctxt_sw_nonv = 0, avg_ctxt_sw_total = 0;
    printf("    context switch volunrary" \
           "  context switch involunrary" \
           "        context switch total" \
           "           cs execution time\n");
    for (int i = 0; i < g_params.n_threads; i++) {
        printf("%28ld ", m_params[i].ctxt_sw_v);
        printf("%27ld ", m_params[i].ctxt_sw_nonv);
        printf("%27ld ", m_params[i].ctxt_sw_total);
        printf("%26.10lf\n", m_params[i].lock_time);
        avg_ctxt_sw_v += m_params[i].ctxt_sw_v;
        avg_ctxt_sw_nonv += m_params[i].ctxt_sw_nonv;
        avg_ctxt_sw_total += m_params[i].ctxt_sw_total;
        avg_lock_time += m_params[i].lock_time;
    }
    printf("============================" \
           "============================" \
           "============================" \
           "===========================\n");
    avg_ctxt_sw_v /= g_params.n_threads;
    avg_ctxt_sw_nonv /= g_params.n_threads;
    avg_ctxt_sw_total /= g_params.n_threads;
    avg_lock_time /= g_params.n_threads;
    printf("%28ld ", avg_ctxt_sw_v);
    printf("%27ld ", avg_ctxt_sw_nonv);
    printf("%27ld ", avg_ctxt_sw_total);
    printf("%26.10lf\n", avg_lock_time);
}

void process_arguments(int argc, char **argv)
{
    int c;
    
    g_params.cs_ptr = cs_methods_array[NORMAL_PTHREAD_MTX];
    g_params.n_threads = 1;
    g_params.affinity = 0;
    
    while (1) {
        int option_index = 0;
        static struct option long_options[] = {
            {"cs-method",required_argument, 0, 'c'},
            {"threads",  required_argument, 0, 't'},
            {"affinity", no_argument,       0, 'a'},
            {"help",     no_argument,       0, 'h'}
        };

        c = getopt_long(argc, argv, "",
                        long_options, &option_index);
        if (c == -1)
            break;

        switch (c) {
        case 'c':
            for (int i = 0; i < CS_METHOD_CNT; i++) {
                if (!strcmp(optarg, cs_method_pnames[i])) {
                    printf("The method %s is selected\n", cs_method_pnames[i]);
                    g_params.cs_ptr = cs_methods_array[i];
                    break;
                }
            }
            break;
        case 't':
            g_params.n_threads = atoi(optarg);
            printf("%d threads will be runned\n", g_params.n_threads);
            break;
        case 'a':
            g_params.affinity = 1;
            printf("Affinity is enabled\n");
            break;
        case 'h':
            printf("./cs-test --threads <number of threads> --cs-method <normal-pthread-mtx, adaptive-pthread-mtx, pthread-spin, smart-mtx, stm>\n");
            exit(EXIT_SUCCESS);
            break;
        default:
            printf("?? getopt returned character code %c ??\n", c);
            printf("./cs-test --threads <number of threads> --cs-method <normal-pthread-mtx, adaptive-pthread-mtx, pthread-spin, smart-mtx, stm>\n");
            exit(EXIT_FAILURE);
        }
    }

}

int main(int argc, char **argv)
{
    long cpu_cnt = sysconf(_SC_NPROCESSORS_ONLN);
    pthread_t *thread_id;
    cpu_set_t cpus;
    pthread_attr_t attr;
    TIMER_T start, stop;

    process_arguments(argc, argv);

    hpctimer_initialize();
    pthread_spin_init(&spin, PTHREAD_PROCESS_PRIVATE);
    pthread_barrier_init(&barrier, NULL, g_params.n_threads);
    pthread_attr_init(&attr);

    m_params = (measured_params *)calloc(sizeof(measured_params), g_params.n_threads);
    t_info = (thread_info *)calloc(sizeof(thread_info), g_params.n_threads);
    thread_id = (pthread_t *)calloc(sizeof(pthread_t), g_params.n_threads);

    prof_array = (int *)malloc(sizeof(int) * g_params.n_threads);
    
    t_info[0].rank = 0;
    thread_id[0] = pthread_self();
    if (g_params.affinity) {
        for (int i = 0; i < g_params.n_threads - 1; i++) {
            t_info[i + 1].rank = i + 1;
            CPU_ZERO(&cpus);
            CPU_SET((i + 1) % cpu_cnt, &cpus);
            pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpus);
            pthread_create(&thread_id[i+1], &attr, thread_fnc, &t_info[i + 1].rank);
        }
        CPU_ZERO(&cpus);
        CPU_SET(0, &cpus);
        pthread_setaffinity_np(thread_id[0], sizeof(cpu_set_t), &cpus);
    } else {
        for (int i = 0; i < g_params.n_threads - 1; i++) {
            t_info[i + 1].rank = i + 1;
            pthread_create(&thread_id[i+1], NULL, thread_fnc, &t_info[i + 1].rank);
        }
    }

    TIMER_READ(start);
    thread_fnc(&t_info[0].rank);
    TIMER_READ(stop);
    for (int i = 0; i < g_params.n_threads - 1; i++) {
        pthread_join(thread_id[i+1], NULL);
    }
    print_measured_params();
    
    pthread_spin_destroy(&spin);
    pthread_barrier_destroy(&barrier);
    pthread_attr_destroy(&attr);
    pthread_mutex_destroy(&m);
    pthread_mutex_destroy(&m_adapt);
    free(m_params);
    free(t_info);
    free(thread_id);
    
    printf("all threads are joined\n");
    printf("total time: %f\n", TIMER_DIFF_SECONDS(start, stop));
    printf("smart_mutex.queue = %d\n", m_smart.__data.queue_length);
    for (int i = 0, summ = 0; i < g_params.n_threads; i++) {
        printf("%5d ", prof_array[i]);
    }
    printf("\n");
    
    free(prof_array);
    
    for (int i = 1, tmp = global_array[0]; i < ARRAY_SIZE; i++) {
        if (tmp != global_array[i]) {
            fprintf(stderr, "%d failed\n", i);
        }
    }
         
    return 0;
}
