#define _GNU_SOURCE
#include <pthread.h>
#include <stdatomic.h>
#include <immintrin.h>
#include <unistd.h>
#include <sched.h>
#include <stdalign.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <sys/syscall.h>
#include <stdint.h>
#include <assert.h>

#include "./include/thread_pool.h"
#include "./include/error.h"

#define CACHE_LINE_SIZE 64
#define ALIGNED(size) alignas(size)
#define CACHE_ALIGNED ALIGNED(CACHE_LINE_SIZE)
#define QUEUE_CAP (1024)
static_assert((QUEUE_CAP & (QUEUE_CAP - 1)) == 0, "QUEUE_CAP must be power of 2");


typedef struct task_handle {
    _Atomic(bool) finished;
    void* result;
} task_handle;

typedef struct {
    void *(*task)(void*);
    void* arg;
    task_handle* handle; 
} task_impl;
typedef _Atomic(uint64_t) a_u_64;


// clang-format off
struct thread_pool {
    CACHE_ALIGNED a_u_64     head;
    CACHE_ALIGNED a_u_64     tail;
    CACHE_ALIGNED task_impl  queue[QUEUE_CAP];
    CACHE_ALIGNED a_u_64     q_lock;
    CACHE_ALIGNED a_u_64  state;
    CACHE_ALIGNED a_u_64  accepting_tasks;
    CACHE_ALIGNED a_u_64  destroy;
    CACHE_ALIGNED a_u_64  inited_threads;
    CACHE_ALIGNED pthread_mutex_t  init_m;
    CACHE_ALIGNED pthread_cond_t   init_c;
    CACHE_ALIGNED pthread_t   start_tread;
    CACHE_ALIGNED pthread_t*  allocated_threads;
    size_t thread_num;
};
// clang-format on

static void* init_thread_func(void* arg);
static void* worker(void* arg);




static _Thread_local int init_clean_up = 1;
#define INIT_CLEAN_UP init_clean_up = 1;
#define INIT_SUCCES(exp) \
    init_clean_up = 0; \
    exp;
static inline void _cleanup_th(thread_pool** th) {
    if (th && *th && init_clean_up) free(*th);
}
//ub then call recursivly
thread_pool* thread_pool_init(size_t thread_num) {
    INIT_CLEAN_UP
    if(thread_num == 0) {
        err_log("thread_num=%lu is less than 1", thread_num);
        return NULL;
    }
    thread_pool* th __attribute__((cleanup(_cleanup_th))) = (thread_pool*)calloc(1, sizeof(thread_pool));
    if(th == NULL) {
        err_log("allocation of %lu bytes failed", sizeof(thread_pool));
        return NULL;
    }
    memset(th, 0, sizeof(thread_pool));
    th->thread_num = thread_num;
    atomic_store(&th->head, 0);
    atomic_store(&th->tail, 0);
    atomic_store(&th->q_lock, 0);
    atomic_store(&th->state, 0);
    atomic_store(&th->destroy, 0);
    atomic_store(&th->inited_threads, 0);
    atomic_store(&th->accepting_tasks, 0);
    if(pthread_create(&th->start_tread, NULL, init_thread_func, th) != 0) {
        err_log("pthread_create failed with error: %s", strerror(errno));
        return NULL;
    }
    INIT_SUCCES(return th);
}




static _Thread_local int init_thread_func_clean_up = 1;
#define INIT_TREAD_FUNC_CLEAN_UP init_thread_func_clean_up = 1;
#define INIT_TREAD_FUNC_SUCCES(exp) \
    init_thread_func_clean_up = 0; \
    exp;
static inline void _cleanup_treads(pthread_t** th) {
    if (th && *th && init_thread_func_clean_up) free(*th);
}
//ub then call recursivly
static void* init_thread_func(void* arg) {
    INIT_TREAD_FUNC_CLEAN_UP
    thread_pool* th = (thread_pool*) arg;
    size_t to_create = th->thread_num - 1; // N-1 cause start tread(current will be last N tread - prevent allocating N+1 threads)
    pthread_t* threads __attribute__((cleanup(_cleanup_treads))) = NULL;
    if(to_create > 0) {
        threads = (pthread_t*)calloc(1, to_create * sizeof(pthread_t));
        if(threads) {
            th->allocated_threads = threads;
        }
        else {
            err_log("Filed to allocate %lu bytes", sizeof(pthread_t) * to_create);
            atomic_store(&th->destroy, 1);
            return NULL;
        }
    }
    pthread_mutex_init(&th->init_m, NULL);
    pthread_cond_init(&th->init_c, NULL);
    for(size_t i = 0; i < to_create; i++) {
        if(pthread_create(&th->allocated_threads[i], NULL, worker, th) != 0) {
            err_log("Failed pthread_create with error: %s", strerror(errno));
            atomic_store_explicit(&th->destroy, 1, memory_order_release);
            atomic_store_explicit(&th->state, 1, memory_order_release);
            pthread_mutex_lock(&th->init_m);
            pthread_cond_broadcast(&th->init_c);
            pthread_mutex_unlock(&th->init_m);
            return NULL;
        }
    }
    INIT_TREAD_FUNC_SUCCES(return worker(th));
}






static inline __attribute__((always_inline)) void acquire_spinlock(a_u_64* lock) {
    uint64_t expected;
    do {
        expected = 0;
        _mm_pause();
    } while (!__atomic_compare_exchange_n(
        lock, &expected, 1, false,
        __ATOMIC_ACQUIRE, __ATOMIC_RELAXED));
}

static inline __attribute__((always_inline)) void release_spinlock(a_u_64* lock) {
    __atomic_clear(lock, __ATOMIC_RELEASE);
}
//posibly can improve perfomance by emnlarging the pause and yelding cpu time to other threads under low contention - prevent to much busy wait in spinlock - for now __mm_pause okey
static void* worker(void* arg) {
    thread_pool* th = (thread_pool*) arg;
    const uint64_t QUEUE_MASK = QUEUE_CAP - 1;
    pthread_mutex_lock(&th->init_m);
    atomic_fetch_add_explicit(&th->inited_threads, 1, memory_order_acq_rel);
    while(
        !atomic_load_explicit(&th->state, memory_order_acquire) &&
        !atomic_load_explicit(&th->destroy, memory_order_acquire)){
            pthread_cond_wait(&th->init_c, &th->init_m);
    }
    pthread_mutex_unlock(&th->init_m);
    if(atomic_load_explicit(&th->destroy, memory_order_acquire) == 1) return NULL;
    uint32_t idle_spins = 0;
    const uint32_t MAX_IDLE_SPINS = 100;
    while(!atomic_load_explicit(&th->destroy, memory_order_acquire)) {
        if(!atomic_load_explicit(&th->state, memory_order_acquire)) {
            idle_spins = 0;
            pthread_mutex_lock(&th->init_m);
            while(!atomic_load_explicit(&th->state, memory_order_acquire) &&
                  !atomic_load_explicit(&th->destroy, memory_order_acquire)) {
                pthread_cond_wait(&th->init_c, &th->init_m);
            }
            pthread_mutex_unlock(&th->init_m);
            if(atomic_load_explicit(&th->destroy, memory_order_acquire)) break;
            continue;
        }
        size_t tail = atomic_load_explicit(&th->tail, memory_order_acquire);
        size_t head = atomic_load_explicit(&th->head, memory_order_acquire);
        if(tail != head) {
            acquire_spinlock(&th->q_lock);
            size_t tail = atomic_load_explicit(&th->tail, memory_order_acquire);
            size_t head = atomic_load_explicit(&th->head, memory_order_acquire);
            if(tail != head) {
                const task_impl task = th->queue[tail & QUEUE_MASK];
                atomic_store_explicit(&th->tail, tail + 1, memory_order_release);
                release_spinlock(&th->q_lock);
                void* result = (void*)task.task(task.arg);
                if(task.handle) {
                    task.handle->result = result;
                    atomic_store_explicit(&task.handle->finished, true, memory_order_release);
                }
                continue;
            }
            release_spinlock(&th->q_lock);
        }
        if(idle_spins < MAX_IDLE_SPINS) {
            _mm_pause();
            idle_spins++;
        } else {
            sched_yield();
            idle_spins = 0;
        }
    }
    return NULL;
}





size_t thread_pool_ready_num(thread_pool* th) {
    if(!th) {
        err_log("arg == NULL");
        return 0;
    }
    return atomic_load_explicit(&th->inited_threads, memory_order_acquire);
}

void thread_pool_activate(thread_pool* th) {
    if(!th) {
        err_log("arg == NULL");
        return;
    }
    pthread_mutex_lock(&th->init_m);
    atomic_store_explicit(&th->accepting_tasks, 1, memory_order_release);
    atomic_store_explicit(&th->state, 1, memory_order_release);
    pthread_cond_broadcast(&th->init_c);
    pthread_mutex_unlock(&th->init_m);
}

void thread_pool_deactivate(thread_pool* th) {
    if(!th) {
        err_log("arg == NULL");
        return;
    }
    atomic_store_explicit(&th->accepting_tasks, 0, memory_order_release);
    thread_pool_drain(th);
    pthread_mutex_lock(&th->init_m);
    atomic_store_explicit(&th->state, 0, memory_order_release);
    pthread_mutex_unlock(&th->init_m);
    _mm_pause();
    sched_yield();
}

bool thread_pool_push_task(thread_pool* th, void *(*task)(void*), void* arg) {
    if(!th || !task) return false;
    if(atomic_load_explicit(&th->destroy, memory_order_acquire)) return false;
    if(!atomic_load_explicit(&th->accepting_tasks, memory_order_acquire)) return false;
    acquire_spinlock(&th->q_lock);
    size_t tail = atomic_load_explicit(&th->tail, memory_order_acquire);
    size_t head = atomic_load_explicit(&th->head, memory_order_acquire);
    int64_t diff = (int64_t)head - (int64_t)tail;
    if(diff >= QUEUE_CAP) {
        release_spinlock(&th->q_lock);
        return false;
    }
    const uint64_t QUEUE_MASK = QUEUE_CAP - 1;
    th->queue[head & QUEUE_MASK].task = task;
    th->queue[head & QUEUE_MASK].arg = arg;
    th->queue[head & QUEUE_MASK].handle = NULL;
    atomic_store_explicit(&th->head, head + 1, memory_order_release);
    release_spinlock(&th->q_lock);
    return true;
}

void thread_pool_destroy(thread_pool* th) {
    if(!th) {
        err_log("arg == NULL");
        return;
    }

    atomic_store_explicit(&th->destroy, 1, memory_order_release);
    atomic_store_explicit(&th->state, 1, memory_order_release);
    pthread_mutex_lock(&th->init_m);
    pthread_cond_broadcast(&th->init_c);
    pthread_mutex_unlock(&th->init_m);
    size_t to_join = th->thread_num - 1;
    for(size_t i = 0; i < to_join; i++) {
        if(th->allocated_threads[i]) pthread_join(th->allocated_threads[i], NULL);
    }
    pthread_join(th->start_tread, NULL);
    pthread_mutex_destroy(&th->init_m);
    pthread_cond_destroy(&th->init_c);
    if(th->allocated_threads) free(th->allocated_threads);
    free(th);
}


static _Thread_local int handle_clean_up = 1;
#define HANDLE_CLEAN_UP handle_clean_up = 1;
#define HANDLE_SUCCES(exp) \
    handle_clean_up = 0; \
    exp;
static inline void _cleanup_handle(task_handle** h) {
    if (h && *h && handle_clean_up) free(*h);
}
//ub then call recursivly
task_handle* thread_pool_push_task_joinable(thread_pool* th, void* (*task)(void*), void* arg) {
    HANDLE_CLEAN_UP
    if(!th || !task) return NULL;
    if(atomic_load_explicit(&th->destroy, memory_order_acquire)) return NULL;
    if(!atomic_load_explicit(&th->accepting_tasks, memory_order_acquire)) return NULL;
    task_handle* __attribute__((cleanup(_cleanup_handle))) handle = (task_handle*)calloc(1, sizeof(task_handle));
    if(!handle){
        err_log("Failed to alloate %lu bytes", sizeof(task_handle));
        return NULL;
    }
    atomic_store_explicit(&handle->finished, false, memory_order_relaxed);
    handle->result = NULL;
    acquire_spinlock(&th->q_lock);
    size_t tail = atomic_load_explicit(&th->tail, memory_order_acquire);
    size_t head = atomic_load_explicit(&th->head, memory_order_acquire);
    int64_t diff = (int64_t)head - (int64_t)tail;
    if(diff >= QUEUE_CAP) {
        release_spinlock(&th->q_lock);
        return NULL;
    }
    const uint64_t QUEUE_MASK = QUEUE_CAP - 1;
    th->queue[head & QUEUE_MASK].task = task;
    th->queue[head & QUEUE_MASK].arg = arg;
    th->queue[head & QUEUE_MASK].handle = handle;
    atomic_store_explicit(&th->head, head + 1, memory_order_release);
    release_spinlock(&th->q_lock);
    HANDLE_SUCCES(return handle);
}

void* thread_pool_task_wait(task_handle* handle) {
    if(!handle) {
        err_log("handle == NULL");
        return NULL;
    }
    
    uint32_t spin_count = 0;
    const uint32_t MAX_SPINS = 1000;
    
    while(!atomic_load_explicit(&handle->finished, memory_order_acquire)) {
        if(spin_count < MAX_SPINS) {
            _mm_pause();
            spin_count++;
        } else {
            sched_yield();
            spin_count = 0;
        }
    }
    
    return handle->result;
}

void thread_pool_task_handle_destroy(task_handle* handle) {
    if(handle) free(handle);
}

void thread_pool_drain(thread_pool* th) {
    if(!th) {
        err_log("arg == NULL");
        return;
    }
    while(1) {
        size_t head = atomic_load_explicit(&th->head, memory_order_acquire);
        size_t tail = atomic_load_explicit(&th->tail, memory_order_acquire);
        if(head == tail) {
            break;
        }
        sched_yield();
        _mm_pause();
    }
}