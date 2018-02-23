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

#ifndef JAEGERTRACINGC_VECTOR_H
#define JAEGERTRACINGC_VECTOR_H

#include "jaegertracingc/alloc.h"
#include "jaegertracingc/common.h"
#include "jaegertracingc/logging.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define JAEGERTRACINGC_VECTOR_RESIZE_FACTOR 2
#define JAEGERTRACINGC_VECTOR_INIT_CAPACITY 10

typedef int (*jaeger_comparator)(const void*, const void*);

typedef struct jaeger_vector {
    int len;
    int capacity;
    void* data;
    int type_size;
    jaeger_allocator* alloc;
} jaeger_vector;

static inline int jaeger_vector_length(const jaeger_vector* vec)
{
    assert(vec != NULL);
    return vec->len;
}

static inline bool jaeger_vector_init(jaeger_vector* vec,
                                      int type_size,
                                      jaeger_allocator* alloc,
                                      jaeger_logger* logger)
{
    assert(vec != NULL);
    assert(type_size > 0);
    if (alloc == NULL) {
        alloc = jaeger_built_in_allocator();
    }
    const int allocated_size = type_size * JAEGERTRACINGC_VECTOR_INIT_CAPACITY;
    void* data = alloc->malloc(alloc, allocated_size);
    if (data == NULL) {
        if (logger != NULL) {
            logger->error(logger,
                          "Failed to initialize vector because initial memory "
                          "could not be allocated");
        }
        return false;
    }
    memset(data, 0, allocated_size);
    *vec = (jaeger_vector){.data = data,
                           .len = 0,
                           .capacity = JAEGERTRACINGC_VECTOR_INIT_CAPACITY,
                           .type_size = type_size,
                           .alloc = alloc};
    return true;
}

static inline void* jaeger_vector_offset(jaeger_vector* vec, int index)
{
    assert(vec != NULL);
    return vec->data + vec->type_size * index;
}

static inline void*
jaeger_vector_get(jaeger_vector* vec, int index, jaeger_logger* logger)
{
    assert(vec != NULL);
    assert(index >= 0);
    if (index >= jaeger_vector_length(vec)) {
        if (logger != NULL) {
            logger->error(logger,
                          "Invalid access to index %d in vector of length %d",
                          index,
                          jaeger_vector_length(vec));
        }
        return NULL;
    }
    return jaeger_vector_offset(vec, index);
}

static inline bool jaeger_vector_reserve(jaeger_vector* vec,
                                         int new_capacity,
                                         jaeger_logger* logger)
{
    assert(vec != NULL);
    if (vec->capacity >= new_capacity) {
        return true;
    }
    int aligned_capacity = vec->capacity;
    while (aligned_capacity < new_capacity) {
        aligned_capacity *= JAEGERTRACINGC_VECTOR_RESIZE_FACTOR;
    }

    void* new_data = vec->alloc->realloc(
        vec->alloc, vec->data, vec->type_size * aligned_capacity);
    if (new_data == NULL) {
        if (logger != NULL) {
            logger->error(logger,
                          "Failed to allocate memory for vector resize, "
                          "current size = %d, new size = %d",
                          vec->type_size * vec->capacity,
                          vec->type_size * aligned_capacity);
        }
        return false;
    }
    void* old_end = new_data + vec->type_size * vec->capacity;
    void* new_end = new_data + vec->type_size * aligned_capacity;
    memset(old_end, 0, new_end - old_end);
    vec->data = new_data;
    vec->capacity = aligned_capacity;
    return true;
}

static inline void* jaeger_vector_extend(jaeger_vector* vec,
                                         int insertion_pos,
                                         int num_elements,
                                         jaeger_logger* logger)
{
    assert(vec != NULL);
    assert(num_elements >= 0);
    assert(jaeger_vector_length(vec) <= vec->capacity);
    if (jaeger_vector_length(vec) + num_elements > vec->capacity &&
        !jaeger_vector_reserve(vec, vec->capacity + num_elements, logger)) {
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

static inline void
jaeger_vector_remove(jaeger_vector* vec, int index, jaeger_logger* logger)
{
    assert(vec != NULL);
    const int len = jaeger_vector_length(vec);
    if (index < 0 || index >= len) {
        if (logger != NULL) {
            logger->error(logger,
                          "Invalid removal of index %d in vector of length %d",
                          index,
                          len);
        }
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

static inline void*
jaeger_vector_insert(jaeger_vector* vec, int index, jaeger_logger* logger)
{
    return jaeger_vector_extend(vec, index, 1, logger);
}

static inline void* jaeger_vector_append(jaeger_vector* vec,
                                         jaeger_logger* logger)
{
    return jaeger_vector_insert(vec, jaeger_vector_length(vec), logger);
}

static inline void jaeger_vector_clear(jaeger_vector* vec)
{
    assert(vec != NULL);
    vec->len = 0;
}

static inline void jaeger_vector_destroy(jaeger_vector* vec)
{
    if (vec != NULL) {
        if (vec->data != NULL) {
            vec->alloc->free(vec->alloc, vec->data);
            vec->data = NULL;
        }
        vec->capacity = 0;
        vec->len = 0;
        vec->type_size = 0;
        vec->alloc = 0;
    }
}

#define JAEGERTRACINGC_VECTOR_FOR_EACH(vec, op)                          \
    do {                                                                 \
        for (int i = 0, len = jaeger_vector_length(vec); i < len; i++) { \
            void* elem = jaeger_vector_offset((vec), i);                 \
            op(elem);                                                    \
        }                                                                \
    } while (0)

static inline void jaeger_vector_sort(jaeger_vector* vec, jaeger_comparator cmp)
{
    qsort(vec->data, jaeger_vector_length(vec), vec->type_size, cmp);
}

static inline void* jaeger_vector_bsearch(jaeger_vector* vec,
                                          const void* key,
                                          jaeger_comparator cmp)
{
    return bsearch(
        key, vec->data, jaeger_vector_length(vec), vec->type_size, cmp);
}

static inline int jaeger_vector_lower_bound(jaeger_vector* vec,
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

#define JAEGERTRACINGC_WRAP_COPY(copy)                          \
    static inline bool copy##_wrapper(void* arg,                \
                                      void* restrict dst,       \
                                      const void* restrict src, \
                                      jaeger_logger* logger)    \
    {                                                           \
        (void) arg;                                             \
        return copy(dst, src, logger);                          \
    }

#define JAEGERTRACINGC_WRAP_DESTROY(destroy)      \
    static inline void destroy##_wrapper(void* x) \
    {                                             \
        destroy(x);                               \
    }

static inline bool jaeger_vector_copy(
    jaeger_vector* restrict dst,
    const jaeger_vector* restrict src,
    bool (*copy)(void*, void* restrict, const void* restrict, jaeger_logger*),
    void* arg,
    jaeger_logger* logger)
{
    assert(dst != NULL);
    assert(src != NULL);
    assert(copy != NULL);
    assert(jaeger_vector_length(dst) == 0);
    if (!jaeger_vector_reserve(dst, jaeger_vector_length(src), logger)) {
        return false;
    }
    for (int i = 0, len = jaeger_vector_length(src); i < len; i++) {
        void* dst_elem = jaeger_vector_append(dst, logger);
        /* Should not be possible to have a NULL returned from append, seeing
         * as all spaced was allocated when jaeger_vector_reserve was called. */
        assert(dst_elem != NULL);
        const void* src_elem = jaeger_vector_offset((jaeger_vector*) src, i);
        assert(src_elem != NULL);
        if (!copy(arg, dst_elem, src_elem, logger)) {
            dst->len--;
            return false;
        }
    }
    return true;
}

typedef struct jaeger_vector_ptr_copy_arg {
    jaeger_allocator* alloc;
    int type_size;
    bool (*copy)(void*, void*, const void*, jaeger_logger*);
    void* arg;
} jaeger_vector_ptr_copy_arg;

static inline bool jaeger_vector_ptr_copy_helper(void* arg,
                                                 void* restrict dst,
                                                 const void* restrict src,
                                                 jaeger_logger* logger)
{
    assert(arg != NULL);
    jaeger_vector_ptr_copy_arg* ctx = arg;
    void** ptr = dst;
    *ptr = ctx->alloc->malloc(ctx->alloc, ctx->type_size);
    if (*ptr == NULL) {
        if (logger != NULL) {
            logger->error(logger,
                          "Cannot allocate %d bytes for vector pointer",
                          ctx->type_size);
        }
        return false;
    }
    if (!ctx->copy(ctx->arg, *ptr, src, logger)) {
        ctx->alloc->free(ctx->alloc, *ptr);
        *ptr = NULL;
        return false;
    }
    return true;
}

static inline bool
jaeger_vector_ptr_copy(jaeger_vector* restrict dst,
                       const jaeger_vector* restrict src,
                       int value_size,
                       bool (*copy)(void*, void*, const void*, jaeger_logger*),
                       void (*dtor)(void*),
                       void* arg,
                       jaeger_logger* logger)
{
    assert(dst != NULL);
    if (dst->type_size != sizeof(void*)) {
        if (logger != NULL) {
            logger->error(logger,
                          "Cannot copy to vector that does not hold pointers, "
                          "type size = %d",
                          dst->type_size);
        }
        return false;
    }
    jaeger_vector_ptr_copy_arg ctx = {dst->alloc, value_size, copy, arg};
    if (!jaeger_vector_copy(
            dst, src, jaeger_vector_ptr_copy_helper, &ctx, logger)) {

#define JAEGERTRACINGC_DST_VECTOR_ALLOC_FREE(x) \
    do {                                        \
        void** ptr = x;                         \
        if (*ptr != NULL) {                     \
            if (dtor != NULL) {                 \
                dtor(*ptr);                     \
            }                                   \
            dst->alloc->free(dst->alloc, *ptr); \
        }                                       \
    } while (0)

        JAEGERTRACINGC_VECTOR_FOR_EACH(dst,
                                       JAEGERTRACINGC_DST_VECTOR_ALLOC_FREE);

#undef JAEGERTRACINGC_DST_VECTOR_ALLOC_FREE

        return false;
    }

    return true;
}

static inline bool jaeger_vector_protobuf_copy(
    void*** restrict dst,
    size_t* n_dst,
    const jaeger_vector* restrict src,
    int value_size,
    bool (*copy)(void*, void*, const void*, jaeger_logger*),
    void (*dtor)(void*),
    void* arg,
    jaeger_logger* logger)
{
    assert(dst != NULL);
    assert(n_dst != NULL);
    jaeger_vector vec;
    if (!jaeger_vector_init(&vec, sizeof(void*), NULL, logger)) {
        return false;
    }
    if (!jaeger_vector_ptr_copy(
            &vec, src, value_size, copy, dtor, arg, logger)) {
        jaeger_vector_destroy(&vec);
        return false;
    }
    *dst = vec.data;
    *n_dst = jaeger_vector_length(&vec);
    return true;
}

#ifdef __cplusplus
} /* extern C */
#endif /* __cplusplus */

#endif /* JAEGERTRACINGC_VECTOR_H */
