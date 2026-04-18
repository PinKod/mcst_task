#include "vectior_v2_impl.h"
#include "./include/thread_pool.h"

#include <stdlib.h>
#include <string.h>
#include <stddef.h>

/* ---------- sequential sort helpers ---------- */

static int cmp_u64(const void* a, const void* b) {
    uint64_t x = *(const uint64_t*)a;
    uint64_t y = *(const uint64_t*)b;
    return (x > y) - (x < y);
}

typedef struct {
    uint64_t* data;
    size_t    size;
} chunk_arg;

static void* sort_chunk(void* arg) {
    chunk_arg* c = (chunk_arg*)arg;
    qsort(c->data, c->size, sizeof(uint64_t), cmp_u64);
    return NULL;
}

/* ---------- min-heap for k-way merge ---------- */

typedef struct {
    uint64_t val;
    size_t   chunk;
} heap_node;

static inline void sift_down(heap_node* h, size_t n, size_t i) {
    for (;;) {
        size_t s = i, l = 2*i+1, r = 2*i+2;
        if (l < n && h[l].val < h[s].val) s = l;
        if (r < n && h[r].val < h[s].val) s = r;
        if (s == i) break;
        heap_node tmp = h[i]; h[i] = h[s]; h[s] = tmp;
        i = s;
    }
}

/* ---------- public API ---------- */

/*
 * Parallel sort: divide array into n_threads chunks, sort each chunk on the
 * thread pool concurrently, then k-way merge with a min-heap.
 * Falls back to plain qsort for tiny arrays or single-thread mode.
 */
void vector_sort_v2(vector_v2* vc, thread_pool* th, size_t n_threads) {
    if (!vc || vc->size <= 1) return;
    uint64_t* data  = vc->data;
    size_t    total = (size_t)vc->size;
    if (n_threads <= 1 || total < 2048) {
        qsort(data, total, sizeof(uint64_t), cmp_u64);
        return;
    }
    if (n_threads > total) n_threads = total;
    size_t chunk_sz = (total + n_threads - 1) / n_threads;
    size_t nchunks  = (total + chunk_sz - 1) / chunk_sz;
    chunk_arg*    args    = malloc(nchunks * sizeof(chunk_arg));
    task_handle** handles = malloc(nchunks * sizeof(task_handle*));
    if (!args || !handles) goto fallback;
    for (size_t i = 0; i < nchunks; i++) {
        size_t start = i * chunk_sz;
        size_t end   = start + chunk_sz;
        if (end > total) end = total;
        args[i].data = data + start;
        args[i].size = end - start;
        handles[i] = thread_pool_push_task_joinable(th, sort_chunk, &args[i]);
        if (!handles[i]) {
            sort_chunk(&args[i]);
        }
    }
    for (size_t i = 0; i < nchunks; i++) {
        if (handles[i]) {
            thread_pool_task_wait(handles[i]);
            thread_pool_task_handle_destroy(handles[i]);
        }
    }
    uint64_t* tmp = malloc(total * sizeof(uint64_t));
    size_t*   pos = calloc(nchunks, sizeof(size_t));
    heap_node* h  = malloc(nchunks * sizeof(heap_node));
    if (!tmp || !pos || !h) {
        free(tmp); free(pos); free(h);
        goto fallback_free;
    }
    size_t hsz = 0;
    for (size_t i = 0; i < nchunks; i++) {
        if (args[i].size > 0) {
            h[hsz].val   = args[i].data[0];
            h[hsz].chunk = i;
            hsz++;
        }
    }
    for (size_t i = hsz / 2; i-- > 0; )
        sift_down(h, hsz, i);
    for (size_t out = 0; hsz > 0; out++) {
        size_t ci = h[0].chunk;
        tmp[out]  = h[0].val;
        pos[ci]++;
        if (pos[ci] < args[ci].size) {
            h[0].val = args[ci].data[pos[ci]];
            sift_down(h, hsz, 0);
        } else {
            h[0] = h[--hsz];
            if (hsz > 0) sift_down(h, hsz, 0);
        }
    }

    memcpy(data, tmp, total * sizeof(uint64_t));

    free(tmp); free(pos); free(h);
    free(args); free(handles);
    return;

fallback_free:
    free(args); free(handles);
fallback:
    qsort(data, total, sizeof(uint64_t), cmp_u64);
}
