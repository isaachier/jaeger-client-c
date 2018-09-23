/*
 * Copyright (c) 2018 The Jaeger Authors.
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

/**
 * @file
 * C generic vector implementation.
 */

#ifndef JAEGERTRACINGC_VECTOR_H
#define JAEGERTRACINGC_VECTOR_H

#include "jaegertracingc/common.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define JAEGERTRACINGC_VECTOR_RESIZE_FACTOR 2
#define JAEGERTRACINGC_VECTOR_INIT_CAPACITY 10

typedef int (*jaeger_comparator)(const void*, const void*);

typedef struct jaeger_vector {
    int len;
    int capacity;
    char* data;
    int type_size;
} jaeger_vector;

#define JAEGERTRACINGC_VECTOR_INIT                            \
    {                                                         \
        .len = 0, .capacity = 0, .data = NULL, .type_size = 0 \
    }

#define JAEGERTRACINGC_VECTOR_FOR_EACH(vec, op, type)                    \
    do {                                                                 \
        for (int i = 0, len = jaeger_vector_length(vec); i < len; i++) { \
            op((type*) jaeger_vector_offset((vec), i));                  \
        }                                                                \
    } while (0)

int jaeger_vector_length(const jaeger_vector* vec);

bool jaeger_vector_init(jaeger_vector* vec, int type_size);

void* jaeger_vector_offset(jaeger_vector* vec, int index);

void* jaeger_vector_get(jaeger_vector* vec, int index);

bool jaeger_vector_reserve(jaeger_vector* vec, int new_capacity);

void* jaeger_vector_extend(jaeger_vector* vec,
                           int insertion_pos,
                           int num_elements);

void jaeger_vector_remove(jaeger_vector* vec, int index);

void* jaeger_vector_insert(jaeger_vector* vec, int index);

void* jaeger_vector_append(jaeger_vector* vec);

void jaeger_vector_clear(jaeger_vector* vec);

void jaeger_vector_destroy(jaeger_vector* vec);

void jaeger_vector_sort(jaeger_vector* vec, jaeger_comparator cmp);

void* jaeger_vector_bsearch(jaeger_vector* vec,
                            const void* key,
                            jaeger_comparator cmp);

int jaeger_vector_lower_bound(jaeger_vector* vec,
                              const void* key,
                              jaeger_comparator cmp);

bool jaeger_vector_copy(jaeger_vector* restrict dst,
                        const jaeger_vector* restrict src,
                        bool (*copy)(void*,
                                     void* restrict,
                                     const void* restrict),
                        void* arg);

bool jaeger_vector_ptr_copy(jaeger_vector* restrict dst,
                            const jaeger_vector* restrict src,
                            int value_size,
                            bool (*copy)(void*, void*, const void*),
                            void (*destroy)(void*),
                            void* arg);

bool jaeger_vector_protobuf_copy(void*** restrict dst,
                                 size_t* n_dst,
                                 const jaeger_vector* restrict src,
                                 int value_size,
                                 bool (*copy)(void*, void*, const void*),
                                 void (*destroy)(void*),
                                 void* arg);

#ifdef __cplusplus
} /* extern C */
#endif /* __cplusplus */

#endif /* JAEGERTRACINGC_VECTOR_H */
