#ifndef VECTOR_V2_H
#define VECTOR_V2_H

#include <stdint.h>
#include <stdlib.h>
#include "thread_pool.h"

typedef struct vector_v2 vector_v2;

vector_v2* in_stream_v2(int fd);
void out_stream_v2(vector_v2* vc, int fd);



void vector_free_v2(vector_v2* vc);
uint64_t vector_size_v2(vector_v2* vc);
uint64_t vector_i_v2(vector_v2* vc, uint64_t i);


static void _vector_free_v2(vector_v2** vc) {
    if(vc && *vc) {
        vector_free_v2(*vc);
    } 
}
#define deffer_free_vector_v2(vc_name) \
    vector_v2* __attribute__((cleanup(_vector_free_v2))) vc_name 



void vector_sort_v2(vector_v2* vc, thread_pool* th, size_t n_threads);

#endif