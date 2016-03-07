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
    NUM_THREADS = 3,
    STR_LNGTH = 32,
    ARRAY_SIZE = 1024,
    NUM_ITERATION = 10240,
};

enum cs_method {
    NORMAL_PTHREAD_MTX = 0,
    ADAPTIVE_PTHREAD_MTX,
    PTHREAD_SPIN,
    CS_METHOD_CNT
};

typedef struct global_params_s {
    int n_threads;
    int affinity;
    double (*cs_ptr)(void);
} global_params;

typedef struct thread_info_s  {
    pid_t pid;
    pid_t tid;
    char status_path[ARRAY_SIZE];
} thread_info;

double cs_normal_pthread_mutex();
double cs_adaptive_pthread_mutex();
double cs_pthread_spin();

static char cs_method_pnames[CS_METHOD_CNT][STR_LNGTH] =
{ "normal-pthread-mtx",
  "adaptive-pthread-mtx",
  "pthread-spin" };
double (*cs_methods_array[CS_METHOD_CNT])() = 
{ cs_normal_pthread_mutex,
  cs_adaptive_pthread_mutex,
  cs_pthread_spin };
static global_params g_params;

pid_t gettid()
{
    return syscall(SYS_gettid);
}

thread_info *thread_info_init()
{
    thread_info *tmp = calloc(1, sizeof(thread_info));

    if (tmp == NULL) {
        fprintf(stderr, "[%s:%d] malloc error\n", __func__, __LINE__);
        exit(EXIT_FAILURE);
    }

    tmp->pid = getpid();
    tmp->tid = gettid();
    sprintf(tmp->status_path, "/proc/%d/task/%d/status", tmp->pid, tmp->tid);
    
    return tmp;
}

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

pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
pthread_spinlock_t spin;
int global_array[ARRAY_SIZE];

/*The the critical section being measured with normal pthread_mutex*/
double cs_normal_pthread_mutex()
{
    double t_total = 0;

    for (int i = 0; i < NUM_ITERATION; i++) {
        double t = 0;

        t = hpctimer_wtime();
        pthread_mutex_lock(&m);
        t_total += hpctimer_wtime() - t;

        global_array[i % ARRAY_SIZE]++;

        pthread_mutex_unlock(&m);
    }

    return t_total;
}

/*The the critical section being measured with pthread_mutex*/
double cs_adaptive_pthread_mutex()
{
    double t_total = 0;
    
    for (int i = 0; i < NUM_ITERATION; i++) {
        double t = 0;

        t = hpctimer_wtime();
        pthread_mutex_lock(&m);
        t_total += hpctimer_wtime() - t;

        global_array[i % ARRAY_SIZE]++;

        pthread_mutex_unlock(&m);
    }

    return t_total;
}

/*The the critical section being measured with pthread_spin*/
double cs_pthread_spin()
{
    double t_total = 0;
    
    for (int i = 0; i < NUM_ITERATION; i++) {
        double t = 0;

        t = hpctimer_wtime();
        pthread_spin_lock(&spin);
        t_total += (hpctimer_wtime() - t);

        global_array[i % ARRAY_SIZE]++;

        pthread_spin_unlock(&spin);
    }

    return t_total;
}


pthread_barrier_t barrier;
void *thread_fnc(void *arg)
{
    double t = 0;
    uint64_t ctxt_sw_v_before = 0, ctxt_sw_v_after = 0;
    uint64_t ctxt_sw_inv_before = 0, ctxt_sw_inv_after = 0;
    uint64_t ctxt_sw_total_before = 0, ctxt_sw_total_after = 0;
    thread_info *ts = thread_info_init();
    
    pthread_barrier_wait(&barrier);

    ctxt_switch_get(ts, &ctxt_sw_v_before, &ctxt_sw_inv_before, &ctxt_sw_total_before);
    t = g_params.cs_ptr();
    ctxt_switch_get(ts, &ctxt_sw_v_after, &ctxt_sw_inv_after, &ctxt_sw_total_after);

    pthread_barrier_wait(&barrier);

    pthread_mutex_lock(&m);
    printf("%27c before  after   diff\n", ' ');
    printf("context switch volunrary:  %7ld %6ld %6ld\n", ctxt_sw_v_before, ctxt_sw_v_after,
        ctxt_sw_v_after - ctxt_sw_v_before);
    printf("context switch involunrary %7ld %6ld %6ld\n", ctxt_sw_inv_before, ctxt_sw_inv_after,
        ctxt_sw_inv_after - ctxt_sw_inv_before);
    printf("context switch total       %7ld %6ld %6ld\n",  ctxt_sw_total_before, ctxt_sw_total_after,
        ctxt_sw_total_after - ctxt_sw_total_before);
    printf("time = %.10lf\n", t / NUM_ITERATION);
    pthread_mutex_unlock(&m);    

    return NULL;
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
            printf("./cs-test --threads <number of threads> --cs-method <normal-pthread-mtx, adaptive-pthread-mtx, pthread-spin>\n");
            exit(EXIT_SUCCESS);
            break;
        default:
            printf("?? getopt returned character code %c ??\n", c);
            printf("./cs-test --threads <number of threads> --cs-method <normal-pthread-mtx, adaptive-pthread-mtx, pthread-spin>\n");
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

    thread_id = (pthread_t *)calloc(sizeof(pthread_t), g_params.n_threads);
    thread_id[0] = pthread_self();
    if (g_params.affinity) {
        for (int i = 0; i < g_params.n_threads - 1; i++) {
            CPU_ZERO(&cpus);
            CPU_SET((i + 1) % cpu_cnt, &cpus);
            pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpus);
            pthread_create(&thread_id[i+1], &attr, thread_fnc, &g_params.n_threads);
        }
        CPU_ZERO(&cpus);
        CPU_SET(0, &cpus);
        pthread_setaffinity_np(thread_id[0], sizeof(cpu_set_t), &cpus);
    } else {
        for (int i = 0; i < g_params.n_threads - 1; i++) {
            pthread_create(&thread_id[i+1], NULL, thread_fnc, &g_params.n_threads);
        }
    }

    TIMER_READ(start);
    thread_fnc(&g_params.n_threads);
    TIMER_READ(stop);
    for (int i = 0; i < g_params.n_threads - 1; i++) {
        pthread_join(thread_id[i+1], NULL);
    }
    free(thread_id);

    pthread_spin_destroy(&spin);
    pthread_barrier_destroy(&barrier);
    pthread_attr_destroy(&attr);
    
    printf("all threads are joined\n");
    printf("total time: %f\n", TIMER_DIFF_SECONDS(start, stop));
    
    return 0;
}
