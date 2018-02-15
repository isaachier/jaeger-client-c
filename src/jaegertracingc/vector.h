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
    if (index >= vec->len) {
        if (logger != NULL) {
            logger->error(logger,
                          "Invalid access to index %d in vector of length %d",
                          index,
                          vec->len);
        }
        return NULL;
    }
    return jaeger_vector_offset(vec, index);
}

static inline int jaeger_vector_length(jaeger_vector* vec)
{
    assert(vec != NULL);
    return vec->len;
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

static inline void*
jaeger_vector_insert(jaeger_vector* vec, int index, jaeger_logger* logger)
{
    assert(vec != NULL);
    assert(vec->len <= vec->capacity);
    if (vec->len == vec->capacity &&
        !jaeger_vector_reserve(
            vec, vec->capacity * JAEGERTRACINGC_VECTOR_RESIZE_FACTOR, logger)) {
        return NULL;
    }
    if (index < vec->len) {
        memmove(jaeger_vector_offset(vec, index + 1),
                jaeger_vector_offset(vec, index),
                vec->type_size * (vec->len - index));
    }
    vec->len++;
    return jaeger_vector_offset(vec, index);
}

static inline void* jaeger_vector_append(jaeger_vector* vec,
                                         jaeger_logger* logger)
{
    return jaeger_vector_insert(vec, vec->len, logger);
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

static inline void*
jaeger_vector_find(jaeger_vector* vec, const void* key, jaeger_comparator cmp)
{
    for (int i = 0; i < vec->len; i++) {
        void* elem = jaeger_vector_offset(vec, i);
        if (memcmp(elem, key, vec->type_size) == 0) {
            return elem;
        }
    }
    return NULL;
}

static inline void jaeger_vector_sort(jaeger_vector* vec, jaeger_comparator cmp)
{
    qsort(vec->data, vec->len, vec->type_size, cmp);
}

static inline void* jaeger_vector_bsearch(jaeger_vector* vec,
                                          const void* key,
                                          jaeger_comparator cmp)
{
    return bsearch(key, vec->data, vec->len, vec->type_size, cmp);
}

static inline int jaeger_vector_lower_bound(jaeger_vector* vec,
                                            const void* key,
                                            jaeger_comparator cmp)
{
    /* Lower bound based on example in
     * http://www.cplusplus.com/reference/algorithm/lower_bound/. */
    int count = vec->len;
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
