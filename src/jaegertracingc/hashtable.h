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

/**
 * @file
 * Hashtable data structure.
 */

#ifndef JAEGERTRACINGC_HASHTABLE_H
#define JAEGERTRACINGC_HASHTABLE_H

#include "jaegertracingc/common.h"
#include "jaegertracingc/key_value.h"
#include "jaegertracingc/list.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * Initial order of new hashtable.
 */
#define JAEGERTRACINGC_HASHTABLE_INIT_ORDER 4u

/**
 * Threshold of size to bucket count ratio at which point the hashtable will
 * rehash into a larger table.
 */
#define JAEGERTRACINGC_HASHTABLE_THRESHOLD 1.0

/**
 * Hashtable data structure.
 */
typedef struct jaeger_hashtable {
    size_t size;
    size_t order;
    jaeger_list* buckets;
} jaeger_hashtable;

/**
 * Static initializer for hashtable.
 */
#define JAEGERTRACINGC_HASHTABLE_INIT          \
    {                                          \
        .size = 0, .order = 0, .buckets = NULL \
    }

/** Define link list node for key-value pairs. */
typedef struct jaeger_key_value_node {
    jaeger_list_node base;
    jaeger_key_value data;
} jaeger_key_value_node;

void jaeger_key_value_node_dealloc(jaeger_destructible* d);

jaeger_key_value_node* jaeger_key_value_node_new(jaeger_key_value data);

/**
 * Clear all entries from a hashtable.
 */
void jaeger_hashtable_clear(jaeger_hashtable* hashtable);

/**
 * Hashtable destructor.
 */
void jaeger_hashtable_destroy(jaeger_hashtable* hashtable);

/**
 * Hashtable constructor.
 */
bool jaeger_hashtable_init(jaeger_hashtable* hashtable);

size_t jaeger_hashtable_hash(const char* key);

bool jaeger_hashtable_rehash(jaeger_hashtable* hashtable);

typedef struct jaeger_hashtable_lookup_result {
    jaeger_key_value_node* node;
    jaeger_list* bucket;
} jaeger_hashtable_lookup_result;

jaeger_hashtable_lookup_result
jaeger_hashtable_internal_lookup(jaeger_hashtable* hashtable, const char* key);

const jaeger_key_value* jaeger_hashtable_find(jaeger_hashtable* hashtable,
                                              const char* key);

bool jaeger_hashtable_put(jaeger_hashtable* hashtable,
                          const char* key,
                          const char* value);

void jaeger_hashtable_remove(jaeger_hashtable* hashtable, const char* key);

uint32_t jaeger_hashtable_minimal_order(uint32_t size);

bool jaeger_hashtable_copy(jaeger_hashtable* restrict dst,
                           const jaeger_hashtable* restrict src);

#ifdef __cplusplus
} /* extern C */
#endif /* __cplusplus */

#endif /* JAEGERTRACINGC_HASHTABLE_H */
