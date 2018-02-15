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

static inline int jaeger_vector_length(jaeger_vector* vec)
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
    void* data =
        alloc->malloc(alloc, type_size * JAEGERTRACINGC_VECTOR_INIT_CAPACITY);
    if (data == NULL) {
        if (logger != NULL) {
            logger->error(logger,
                          "Failed to initialize vector because initial memory "
                          "could not be allocated");
        }
        return false;
    }
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

    void* new_data = vec->alloc->realloc(
        vec->alloc, vec->data, vec->type_size * new_capacity);
    if (new_data == NULL) {
        if (logger != NULL) {
            logger->error(logger,
                          "Failed to allocate memory for vector resize, "
                          "current size = %d, new size = %d",
                          vec->type_size * vec->capacity,
                          vec->type_size * new_capacity);
        }
        return false;
    }
    vec->data = new_data;
    vec->capacity = new_capacity;
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
    if (jaeger_vector_length(vec) + num_elements >= vec->capacity &&
        !jaeger_vector_reserve(
            vec,
            JAEGERTRACINGC_MAX(vec->capacity *
                                   JAEGERTRACINGC_VECTOR_RESIZE_FACTOR,
                               vec->capacity + num_elements),
            logger)) {
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

#ifdef __cplusplus
} /* extern C */
#endif /* __cplusplus */

#endif /* JAEGERTRACINGC_VECTOR_H */
