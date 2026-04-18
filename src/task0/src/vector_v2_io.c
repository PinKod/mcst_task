#include <stdlib.h>
#include <unistd.h>
#include <errno.h>


#include "./include/error.h"
#include "./include/vector_v2.h"
#include "vectior_v2_impl.h"

#define INITIAL_CAPACITY 8192
#define BUFFER_SIZE 65536

static char buffer[BUFFER_SIZE] = {0};



static inline int vector_v2_ensure_capacity(vector_v2* vc) {
    uint64_t new_capacity = vc->capacity << 1;
    uint64_t* new_data = (uint64_t*)realloc(vc->data, new_capacity * sizeof(uint64_t));
    if (new_data == NULL) {
        err_log("Failed to reallocate vector");
        return -1;
    }
    vc->data = new_data;
    vc->capacity = new_capacity;
    return 0;
}

vector_v2* in_stream_v2(int fd) {
    vector_v2* vc = (vector_v2*)malloc(sizeof(vector_v2));
    if (vc == NULL) {
        err_log("Failed to allocate vector");
        return NULL;
    }
    
    vc->data = (uint64_t*)malloc(INITIAL_CAPACITY * sizeof(uint64_t));
    if (vc->data == NULL) {
        err_log("Failed to allocate vector data");
        free(vc);
        return NULL;
    }
    vc->size = 0;
    vc->capacity = INITIAL_CAPACITY;
    
    ssize_t bytes_read;
    uint64_t current_num = 0;
    int in_number = 0;
    
    while ((bytes_read = read(fd, buffer, BUFFER_SIZE)) > 0) {
        register char* ptr = buffer;
        register char* end = buffer + bytes_read;
        
        while (ptr < end) {
            register char c = *ptr++;
            
            if (c >= '0' && c <= '9') {
                current_num = current_num * 10 + (uint64_t)(c - '0');
                in_number = 1;
            } else if (in_number) {
                if (__builtin_expect(vc->size >= vc->capacity, 0)) {
                    if (vector_v2_ensure_capacity(vc) < 0) {
                        vector_free_v2(vc);
                        return NULL;
                    }
                }
                vc->data[vc->size++] = current_num;
                current_num = 0;
                in_number = 0;
            }
        }
    }
    
    if (in_number) {
        if (vc->size >= vc->capacity) {
            if (vector_v2_ensure_capacity(vc) < 0) {
                vector_free_v2(vc);
                return NULL;
            }
        }
        vc->data[vc->size++] = current_num;
    }
    
    if (bytes_read < 0) {
        err_log("Failed to read from file descriptor");
        vector_free_v2(vc);
        return NULL;
    }
    
    return vc;
}

void vector_free_v2(vector_v2* vc) {
    if (vc != NULL) {
        if (vc->data != NULL) {
            free(vc->data);
        }
        free(vc);
    }
}



static inline ssize_t write_all(int fd, const void* buf, size_t count) {
    ssize_t written = 0;
    const char* p = (const char*)buffer;
    while (written < (ssize_t)count) {
        ssize_t res = write(fd, p + written, count - written);
        if (res < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        written += res;
    }
    return written;
}


void out_stream_v2(vector_v2* vc, int fd) {
    if(!vc || !vc->data || vc->size == 0) return;

    size_t pos = 0;
    for(uint64_t i = 0; i < vc->size; i++) {
        uint64_t vc_i = vc->data[i];
        char tmp[24] = {0};
        int len = 0;
        if(vc_i == 0) {
            tmp[0] = '0';
            len = 1;
        }
        else {
            while(vc_i > 0) {
                tmp[len++] = (vc_i % 10) + '0';
                vc_i /= 10;
            }
        }
        if(pos + len + 1 > BUFFER_SIZE) {
            if(write_all(fd, buffer, pos) < 0) return;
            pos = 0;
        }
        for(int j = len - 1; j >= 0; --j) {
            buffer[pos++] = tmp[j];
        }
        buffer[pos++] = ' ';
    }
    write_all(fd, buffer, pos);
}