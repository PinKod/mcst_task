#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <stddef.h>
#include <stdbool.h>

typedef struct thread_pool thread_pool;
typedef struct task_handle task_handle;
typedef void (*thread_task)(void*);

//ub then call recursivly
thread_pool* thread_pool_init(size_t thread_num);

size_t thread_pool_ready_num(thread_pool* th);
void thread_pool_activate(thread_pool* th);
void thread_pool_deactivate(thread_pool* th);
bool thread_pool_push_task(thread_pool* th, void *(*task)(void*), void* arg);
task_handle* thread_pool_push_task_joinable(thread_pool* th, void* (*task)(void*), void* arg);
void* thread_pool_task_wait(task_handle* handle);
void thread_pool_task_handle_destroy(task_handle* handle);
void thread_pool_drain(thread_pool* th);
void thread_pool_destroy(thread_pool* th);

#endif