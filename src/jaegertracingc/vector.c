/*
 * Copyright (c) 2018 Uber Technologies, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "jaegertracingc/vector.h"

int jaeger_vector_length(const jaeger_vector* vec)
{
    assert(vec != NULL);
    return vec->len;
}

bool jaeger_vector_init(jaeger_vector* vec, int type_size)
{
    assert(vec != NULL);
    assert(type_size > 0);
    const int allocated_size = type_size * JAEGERTRACINGC_VECTOR_INIT_CAPACITY;
    char* data = jaeger_malloc(allocated_size);
    if (data == NULL) {
        jaeger_log_error("Failed to initialize vector because initial memory "
                         "could not be allocated");
        return false;
    }
    memset(data, 0, allocated_size);
    *vec = (jaeger_vector){.data = data,
                           .len = 0,
                           .capacity = JAEGERTRACINGC_VECTOR_INIT_CAPACITY,
                           .type_size = type_size};
    return true;
}

void* jaeger_vector_offset(jaeger_vector* vec, int index)
{
    assert(vec != NULL);
    return vec->data + vec->type_size * index;
}

void* jaeger_vector_get(jaeger_vector* vec, int index)
{
    assert(vec != NULL);
    assert(index >= 0);
    if (index >= jaeger_vector_length(vec)) {
        jaeger_log_error("Invalid access to index %d in vector of length %d",
                         index,
                         jaeger_vector_length(vec));
        return NULL;
    }
    return jaeger_vector_offset(vec, index);
}

bool jaeger_vector_reserve(jaeger_vector* vec, int new_capacity)
{
    assert(vec != NULL);
    assert(vec->capacity > 0);
    if (vec->capacity >= new_capacity) {
        return true;
    }
    int aligned_capacity = vec->capacity;
    while (aligned_capacity < new_capacity) {
        aligned_capacity *= JAEGERTRACINGC_VECTOR_RESIZE_FACTOR;
    }

    char* new_data =
        jaeger_realloc(vec->data, vec->type_size * aligned_capacity);
    if (new_data == NULL) {
        jaeger_log_error("Failed to allocate memory for vector resize, "
                         "current size = %d, new size = %d",
                         vec->type_size * vec->capacity,
                         vec->type_size * aligned_capacity);
        return false;
    }
    char* old_end = new_data + vec->type_size * vec->capacity;
    char* new_end = new_data + vec->type_size * aligned_capacity;
    memset(old_end, 0, new_end - old_end);
    vec->data = new_data;
    vec->capacity = aligned_capacity;
    return true;
}

void* jaeger_vector_extend(jaeger_vector* vec,
                           int insertion_pos,
                           int num_elements)
{
    assert(vec != NULL);
    assert(num_elements >= 0);
    assert(jaeger_vector_length(vec) <= vec->capacity);
    if (jaeger_vector_length(vec) + num_elements > vec->capacity &&
        !jaeger_vector_reserve(vec, vec->capacity + num_elements)) {
        return NULL;
    }
    if (insertion_pos < vec->len) {
        memmove(jaeger_vector_offset(vec, insertion_pos + num_elements),
                jaeger_vector_offset(vec, insertion_pos),
                vec->type_size * (vec->len - insertion_pos));
    }
    void* offset = jaeger_vector_offset(vec, insertion_pos);
    vec->len += num_elements;
    return offset;
}

void jaeger_vector_remove(jaeger_vector* vec, int index)
{
    assert(vec != NULL);
    const int len = jaeger_vector_length(vec);
    if (index < 0 || index >= len) {
        jaeger_log_error(
            "Invalid removal of index %d in vector of length %d", index, len);
        return;
    }

    if (index < len - 1) {
        const void* end = jaeger_vector_offset(vec, len);
        void* removed_ptr = jaeger_vector_offset(vec, index);
        void* new_ptr = removed_ptr + vec->type_size;
        memmove(removed_ptr, new_ptr, end - new_ptr);
    }
    vec->len--;
}

void* jaeger_vector_insert(jaeger_vector* vec, int index)
{
    return jaeger_vector_extend(vec, index, 1);
}

void* jaeger_vector_append(jaeger_vector* vec)
{
    return jaeger_vector_insert(vec, jaeger_vector_length(vec));
}

void jaeger_vector_clear(jaeger_vector* vec)
{
    assert(vec != NULL);
    vec->len = 0;
}

void jaeger_vector_destroy(jaeger_vector* vec)
{
    if (vec != NULL) {
        if (vec->data != NULL) {
            jaeger_free(vec->data);
            vec->data = NULL;
        }
        vec->capacity = 0;
        vec->len = 0;
        vec->type_size = 0;
    }
}

void jaeger_vector_sort(jaeger_vector* vec, jaeger_comparator cmp)
{
    qsort(vec->data, jaeger_vector_length(vec), vec->type_size, cmp);
}

void* jaeger_vector_bsearch(jaeger_vector* vec,
                            const void* key,
                            jaeger_comparator cmp)
{
    return bsearch(
        key, vec->data, jaeger_vector_length(vec), vec->type_size, cmp);
}

int jaeger_vector_lower_bound(jaeger_vector* vec,
                              const void* key,
                              jaeger_comparator cmp)
{
    /* Lower bound based on example in
     * http://www.cplusplus.com/reference/algorithm/lower_bound/. */
    int count = jaeger_vector_length(vec);
    int pos = 0;
    while (count > 0) {
        const int step = count / 2;
        pos += step;
        if (cmp(jaeger_vector_offset(vec, pos), key) < 0) {
            pos++;
            count -= step + 1;
        }
        else {
            count = step;
        }
    }
    return pos;
}

bool jaeger_vector_copy(jaeger_vector* restrict dst,
                        const jaeger_vector* restrict src,
                        bool (*copy)(void*,
                                     void* restrict,
                                     const void* restrict),
                        void* arg)
{
    assert(dst != NULL);
    assert(src != NULL);
    assert(copy != NULL);
    assert(jaeger_vector_length(dst) == 0);
    if (!jaeger_vector_reserve(dst, jaeger_vector_length(src))) {
        return false;
    }
    for (int i = 0, len = jaeger_vector_length(src); i < len; i++) {
        void* dst_elem = jaeger_vector_append(dst);
        /* Should not be possible to have a NULL returned from append, seeing
         * as all spaced was allocated when jaeger_vector_reserve was called. */
        assert(dst_elem != NULL);
        const void* src_elem = jaeger_vector_offset((jaeger_vector*) src, i);
        assert(src_elem != NULL);
        if (!copy(arg, dst_elem, src_elem)) {
            dst->len--;
            return false;
        }
    }
    return true;
}

typedef struct jaeger_vector_ptr_copy_arg {
    int type_size;
    bool (*copy)(void*, void*, const void*);
    void* arg;
} jaeger_vector_ptr_copy_arg;

static inline bool jaeger_vector_ptr_copy_helper(void* arg,
                                                 void* restrict dst,
                                                 const void* restrict src)
{
    assert(arg != NULL);
    jaeger_vector_ptr_copy_arg* ctx = (jaeger_vector_ptr_copy_arg*) arg;
    void** ptr = (void**) dst;
    *ptr = (void**) jaeger_malloc(ctx->type_size);
    if (*ptr == NULL) {
        jaeger_log_error("Cannot allocate %d bytes for vector pointer",
                         ctx->type_size);
        return false;
    }
    if (!ctx->copy(ctx->arg, *ptr, src)) {
        jaeger_free(*ptr);
        *ptr = NULL;
        return false;
    }
    return true;
}

bool jaeger_vector_ptr_copy(jaeger_vector* restrict dst,
                            const jaeger_vector* restrict src,
                            int value_size,
                            bool (*copy)(void*, void*, const void*),
                            void (*destroy)(void*),
                            void* arg)
{
    assert(dst != NULL);
    if (dst->type_size != sizeof(void*)) {
        jaeger_log_error("Cannot copy to vector that does not hold pointers, "
                         "type size = %d",
                         dst->type_size);
        return false;
    }
    jaeger_vector_ptr_copy_arg ctx = {value_size, copy, arg};
    if (!jaeger_vector_copy(dst, src, jaeger_vector_ptr_copy_helper, &ctx)) {

#define JAEGERTRACINGC_DST_VECTOR_ALLOC_FREE(ptr) \
    do {                                          \
        if (*(ptr) != NULL) {                     \
            if (destroy != NULL) {                \
                destroy(*(ptr));                  \
            }                                     \
            jaeger_free(*(ptr));                  \
        }                                         \
    } while (0)

        JAEGERTRACINGC_VECTOR_FOR_EACH(
            dst, JAEGERTRACINGC_DST_VECTOR_ALLOC_FREE, void**);

#undef JAEGERTRACINGC_DST_VECTOR_ALLOC_FREE

        return false;
    }

    return true;
}

bool jaeger_vector_protobuf_copy(void*** restrict dst,
                                 size_t* n_dst,
                                 const jaeger_vector* restrict src,
                                 int value_size,
                                 bool (*copy)(void*, void*, const void*),
                                 void (*destroy)(void*),
                                 void* arg)
{
    assert(dst != NULL);
    assert(n_dst != NULL);
    if (jaeger_vector_length(src) == 0) {
        *dst = NULL;
        *n_dst = 0;
        return true;
    }
    jaeger_vector vec;
    if (!jaeger_vector_init(&vec, sizeof(void*))) {
        return false;
    }
    if (!jaeger_vector_ptr_copy(&vec, src, value_size, copy, destroy, arg)) {
        jaeger_vector_destroy(&vec);
        return false;
    }
    *dst = (void**) vec.data;
    *n_dst = jaeger_vector_length(&vec);
    return true;
}
