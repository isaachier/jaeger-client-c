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

static inline void* jaeger_vector_append(jaeger_vector* vec,
                                         jaeger_logger* logger)
{
    assert(vec != NULL);
    assert(vec->len <= vec->capacity);
    if (vec->len == vec->capacity) {
        const int new_capacity =
            vec->capacity * JAEGERTRACINGC_VECTOR_RESIZE_FACTOR;
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
            return NULL;
        }
        vec->data = new_data;
        vec->capacity = new_capacity;
    }
    vec->len++;
    return (vec->data + vec->type_size * (vec->len - 1));
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
    return (vec->data + vec->type_size * index);
}

static inline int jaeger_vector_length(jaeger_vector* vec)
{
    assert(vec != NULL);
    return vec->len;
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

#ifdef __cplusplus
} /* extern C */
#endif /* __cplusplus */

#endif /* JAEGERTRACINGC_VECTOR_H */
