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
#define JAEGERTRACINGC_HASHTABLE_INIT(hashtable)                         \
    {                                                                    \
        .size = 0, .order = 0, .buckets = NULL,                          \
        .list = JAEGERTRACINGC_HASHTABLE_LIST_INIT((hashtable).list),    \
        .ordered_list =                                                  \
            JAEGERTRACINGC_HASHTABLE_LIST_INIT((hashtable).ordered_list) \
    }

#define JAEGERTRACINGC_HASHTABLE_KEY_TO_ITER(k) \
    JAEGERTRACINGC_CONTAINER_OF(k, jaeger_hashtable_entry, key)->ordered_list

/**
 * Clear all entries from a hashtable.
 */
static inline void jaeger_hashtable_clear(jaeger_hashtable* hashtable)
{
    for (jaeger_hashtable_list *list = &hashtable->list, *next;
         list != &hashtable->list;
         list = next) {
        next = list->next;
        jaeger_hashtable_entry* entry =
            JAEGERTRACINGC_CONTAINER_OF(list, jaeger_hashtable_entry, list);
        jaeger_hashtable_entry_destroy(entry);
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

static inline size_t
jaeger_hashtable_bucket_count(const jaeger_hashtable* hashtable)
{
    assert(hashtable != NULL);
    return (((size_t) 1) << hashtable->order);
}

static inline size_t jaeger_hashtable_bitmask(const jaeger_hashtable* hashtable)
{
    assert(hashtable != NULL);
    return jaeger_hashtable_bucket_count(hashtable) - 1;
}

static inline bool
jaeger_hashtable_bucket_is_empty(const jaeger_hashtable* hashtable,
                                 const jaeger_hashtable_bucket* bucket)
{
    return bucket->first == &hashtable->list && bucket->first == bucket->last;
}

static inline void
jaeger_hashtable_bucket_insert(jaeger_hashtable* hashtable,
                               jaeger_hashtable_bucket* bucket,
                               jaeger_hashtable_list* list)
{
    if (jaeger_hashtable_bucket_is_empty(hashtable, bucket)) {
        jaeger_hashtable_list_insert(&hashtable->list, list);
        bucket->first = list;
        bucket->last = list;
        return;
    }

    jaeger_hashtable_list_insert(bucket->first, list);
    bucket->first = list;
}

/**
 * Hashtable constructor.
 */
static inline bool jaeger_hashtable_init(jaeger_hashtable* hashtable)
{
    assert(hashtable != NULL);
    *hashtable = (jaeger_hashtable) JAEGERTRACINGC_HASHTABLE_INIT(*hashtable);
    const size_t init_capacity = jaeger_hashtable_bucket_count(hashtable);
    hashtable->buckets =
        jaeger_malloc(init_capacity * sizeof(jaeger_hashtable_bucket));
    memset(hashtable->buckets, 0, sizeof(jaeger_hashtable_bucket));
    if (hashtable->buckets == NULL) {
        jaeger_hashtable_destroy(hashtable);
        return false;
    }
    hashtable->size = 0;
    hashtable->order = JAEGERTRACINGC_HASHTABLE_INIT_ORDER;
    for (size_t i = 0; i < init_capacity; i++) {
        hashtable->buckets[i] =
            (jaeger_hashtable_bucket) JAEGERTRACINGC_HASHTABLE_BUCKET_INIT(
                *hashtable);
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

static inline jaeger_hashtable_entry*
jaeger_hashtable_find_entry(jaeger_hashtable* hashtable,
                            jaeger_hashtable_bucket* bucket,
                            const char* key,
                            size_t hash_code)
{
    /* TODO */
    return NULL;
}

static inline bool jaeger_hashtable_put(jaeger_hashtable* hashtable,
                                        const char* key,
                                        const char* value)
{
    if (((double) hashtable->size + 1) /
                jaeger_hashtable_bucket_count(hashtable) >
            JAEGERTRACINGC_HASHTABLE_THRESHOLD &&
        !jaeger_hashtable_rehash(hashtable)) {
        return false;
    }

    const size_t hash_code = jaeger_hashtable_hash(key);
    const size_t index = hash_code & jaeger_hashtable_bitmask(hashtable);
    jaeger_hashtable_bucket* bucket = &hashtable->buckets[index];
    jaeger_hashtable_entry* entry =
        jaeger_hashtable_find_entry(hashtable, bucket, key, hash_code);

    if (entry != NULL) {
        char* value_copy = jaeger_strdup(value);
        if (value_copy == NULL) {
            return false;
        }
        jaeger_free(entry->value);
        entry->value = value_copy;
        return true;
    }

    entry = jaeger_malloc(sizeof(*entry));
    if (entry == NULL) {
        return false;
    }
    memset(entry, 0, sizeof(*entry));
    entry->key = jaeger_strdup(key);
    if (entry->key == NULL) {
        goto cleanup;
    }
    entry->value = jaeger_strdup(value);
    if (entry->value == NULL) {
        goto cleanup;
    }
    entry->hash_code = hash_code;
    entry->list =
        (jaeger_hashtable_list) JAEGERTRACINGC_HASHTABLE_LIST_INIT(entry->list);
    entry->ordered_list =
        (jaeger_hashtable_list) JAEGERTRACINGC_HASHTABLE_LIST_INIT(
            entry->ordered_list);

    jaeger_hashtable_bucket_insert(hashtable, bucket, &entry->list);
    jaeger_hashtable_list_insert(&hashtable->ordered_list, &entry->list);
    hashtable->size++;

    return true;

cleanup:
    jaeger_free(entry->key);
    jaeger_free(entry->value);
    jaeger_free(entry);
    return false;
}

#ifdef __cplusplus
} /* extern C */
#endif /* __cplusplus */

#endif /* JAEGERTRACINGC_HASHTABLE_H */
