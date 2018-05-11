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
#define JAEGERTRACINGC_HASHTABLE_INIT_ORDER 4

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
#define JAEGERTRACINGC_HASHTABLE_INIT(hashtable) \
    {                                            \
        .size = 0, .order = 0, .buckets = NULL   \
    }

JAEGERTRACINGC_DEFINE_LIST_NODE(jaeger_key_value_node,
                                jaeger_key_value,
                                jaeger_key_value_destroy)

/**
 * Clear all entries from a hashtable.
 */
static inline void jaeger_hashtable_clear(jaeger_hashtable* hashtable)
{
    assert(hashtable != NULL);
    const size_t capacity = (1 << hashtable->order);
    for (size_t i = 0; i < capacity; i++) {
        jaeger_list_clear(&hashtable->buckets[i]);
    }
}

/**
 * Hashtable destructor.
 */
static inline void jaeger_hashtable_destroy(jaeger_hashtable* hashtable)
{
    if (hashtable == NULL) {
        return;
    }
    jaeger_hashtable_clear(hashtable);
    free(hashtable->buckets);
    hashtable->buckets = NULL;
}

/**
 * Hashtable constructor.
 */
static inline bool jaeger_hashtable_init(jaeger_hashtable* hashtable)
{
    assert(hashtable != NULL);
    *hashtable = (jaeger_hashtable) JAEGERTRACINGC_HASHTABLE_INIT(*hashtable);
    const size_t init_capacity = (1 << JAEGERTRACINGC_HASHTABLE_INIT_ORDER);
    hashtable->buckets = jaeger_malloc(init_capacity * sizeof(jaeger_list));
    if (hashtable->buckets == NULL) {
        jaeger_hashtable_destroy(hashtable);
        return false;
    }
    memset(hashtable->buckets, 0, init_capacity * sizeof(jaeger_list));
    hashtable->size = 0;
    hashtable->order = JAEGERTRACINGC_HASHTABLE_INIT_ORDER;
    for (size_t i = 0; i < init_capacity; i++) {
        hashtable->buckets[i] = (jaeger_list) JAEGERTRACINGC_LIST_INIT;
    }
    return true;
}

static inline bool jaeger_hashtable_rehash(jaeger_hashtable* hashtable)
{
    /* TODO */
    (void) hashtable;
    return true;
}

size_t jaeger_hashtable_hash(const char* key);

static inline jaeger_key_value_node*
jaeger_hashtable_bucket_find(jaeger_list* bucket, const char* key)
{
    assert(bucket != NULL);
    assert(key != NULL);
    for (jaeger_list_node* node = bucket->head; node != NULL;
         node = node->next) {
        if (strcmp(((jaeger_key_value_node*) node)->data.key, key) == 0) {
            return (jaeger_key_value_node*) node;
        }
    }
    return NULL;
}

static inline bool jaeger_hashtable_put(jaeger_hashtable* hashtable,
                                        const char* key,
                                        const char* value)
{
    assert(hashtable != NULL);
    assert(key != NULL);
    assert(value != NULL);
    if (((double) hashtable->size + 1) / (1 << hashtable->order) >
            JAEGERTRACINGC_HASHTABLE_THRESHOLD &&
        !jaeger_hashtable_rehash(hashtable)) {
        return false;
    }

    const size_t hash_code = jaeger_hashtable_hash(key);
    const size_t index = hash_code & ((1 << hashtable->order) - 1);
    jaeger_list* bucket = &hashtable->buckets[index];
    jaeger_key_value_node* entry = jaeger_hashtable_bucket_find(bucket, key);
    if (entry != NULL) {
        char* value_copy = jaeger_strdup(value);
        if (value_copy == NULL) {
            return false;
        }
        jaeger_free(entry->data.value);
        entry->data.value = value_copy;
        return true;
    }

    jaeger_key_value kv;
    if (!jaeger_key_value_init(&kv, key, value)) {
        return false;
    }
    entry = jaeger_key_value_node_new(kv);
    if (entry == NULL) {
        goto cleanup;
    }
    jaeger_list_insert(bucket, bucket->size, (jaeger_list_node*) entry);
    hashtable->size++;

    return true;

cleanup:
    ((jaeger_destructible*) entry)->destroy((jaeger_destructible*) entry);
    return false;
}

#ifdef __cplusplus
} /* extern C */
#endif /* __cplusplus */

#endif /* JAEGERTRACINGC_HASHTABLE_H */
