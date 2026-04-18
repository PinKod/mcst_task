#include "vectior_v2_impl.h"


uint64_t vector_size_v2(vector_v2* vc) {
    return vc->size;
}

uint64_t vector_i_v2(vector_v2* vc, uint64_t i) {
    return vc->data[i];
}