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
 * Hashtable data structure. Implementation inspired by Jansson's hashtable.
 */

#ifndef JAEGERTRACINGC_HASHTABLE_H
#define JAEGERTRACINGC_HASHTABLE_H

#include "jaegertracingc/common.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * Initial order of new hashtable.
 */
#define JAEGERTRACINGC_HASHTABLE_INIT_ORDER 4

/**
 * Linked list used for hashtable chaining.
 */
typedef struct jaeger_hashtable_list {
    /** Pointer to previous node. */
    struct jaeger_hashtable_list* prev;
    /** Pointer to next node. */
    struct jaeger_hashtable_list* next;
} jaeger_hashtable_node;

/**
 * Static initializer for hashtable list.
 */
#define JAEGERTRACINGC_HASHTABLE_LIST_INIT(list) \
    {                                            \
        .prev = (list), .next = (list)           \
    }

/**
 * Entry representing a key-value pair.
 */
typedef struct jaeger_hashtable_entry {
    jaeger_hashtable_list list;
    jaeger_hashtable_list ordered_list;
    size_t hash_code;
    char* key;
    char* value;
} jaeger_hashtable_entry;

/**
 * Static initializer for hashtable entry.
 */
#define JAEGERTRACINGC_HASHTABLE_ENTRY_INIT(entry)                     \
    {                                                                  \
        .list = JAEGERTRACINGC_HASHTABLE_LIST_INIT(&(entry).list),     \
        .ordered_list =                                                \
            JAEGERTRACINGC_HASHTABLE_LIST_INIT(&(entry).ordered_list), \
        .hash_code = 0, .key = NULL, .value = NULL                     \
    }

/**
 * Hashtable entry destructor.
 */
static inline void jaeger_hashtable_entry_destroy(jaeger_hashtable_entry* entry)
{
    if (entry == NULL) {
        return;
    }

    jaeger_hashtable_list_destroy(&entry->list);
    jaeger_hashtable_list_destroy(&entry->ordered_list);
    entry->hash_code = 0;
    jaeger_free(entry->key);
    entry->key = NULL;
    jaeger_free(entry->value);
    entry->value = NULL;
}

/**
 * Hashtable entry constructor.
 * @param key Key for new entry. Must not be NULL.
 * @param value Value for new entry. Must not be NULL.
 * @return True on success, false otherwise.
 */
static inline bool jaeger_hashtable_entry_init(jaeger_hashtable_entry* entry,
                                               const char* key,
                                               const char* value)
{
    assert(entry != NULL);
    assert(key != NULL);
    assert(value != NULL);
    *entry =
        (jaeger_hashtable_entry) JAEGERTRACINGC_HASHTABLE_ENTRY_INIT(*entry);
    entry->key = jaeger_strdup(key);
    if (entry->key == NULL) {
        return false;
    }
    entry->value = jaeger_strdup(value);
    if (entry->value == NULL) {
        jaeger_hashtable_entry_destroy(entry);
        return false;
    }
    return true;
}

/**
 * Hashtable bucket representation.
 */
typedef struct jaeger_hashtable_bucket {
    jaeger_hashtable_list* first;
    jaeger_hashtable_list* last;
} jaeger_hashtable_bucket;

/**
 * Static initializer for hashtable bucket.
 */
#define JAEGERTRACINGC_HASHTABLE_BUCKET_INIT(hashtable)           \
    {                                                             \
        .first = (&(hashtable).list), .last = (&(hashtable).list) \
    }

/**
 * Hashtable data structure.
 */
typedef struct jaeger_hashtable {
    size_t size;
    size_t order;
    jaeger_hashtable_bucket* buckets;
    jaeger_hashtable_list list;
    jaeger_hashtable_list ordered_list;
} jaeger_hashtable;

/**
 * Static initializer for hashtable.
 */
#define JAEGERTRACINGC_HASHTABLE_INIT(hashtable)                       \
    {                                                                  \
        .size = 0, .order = 0, .buckets = NULL,                        \
        .list = JAEGERTRACINGC_HASHTABLE_LIST_INIT(hashtable.list),    \
        .ordered_list =                                                \
            JAEGERTRACINGC_HASHTABLE_LIST_INIT(hashtable.ordered_list) \
    }

#define JAEGERTRACINGC_HASHTABLE_KEY_TO_ITER(k) \
    (&(container_of(k, jaeger_hashtable_entry, key)->ordered_list))

/**
 * Hashtable destructor.
 */
static inline void jaeger_hashtable_destroy(jaeger_hashtable* hashtable)
{
    if (hashtable == NULL) {
        return;
    }

    for (jaeger_hashtable_list *list = hashtable->list, next;
         list != &hashtable->list;
         list = next) {
        next = list->next;
        entry = container_of(list, jaeger_hashtable_entry, list);
        jaeger_hashtable_entry_destroy(entry);
    }
    free(hashtable->buckets);
    hashtable->buckets = NULL;
}

/**
 * Hashtable constructor.
 */
static inline bool jaeger_hashtable_init(jaeger_hashtable* hashtable)
{
    assert(hashtable != NULL);
    *hashtable = (jaeger_hashtable) JAEGERTRACINGC_HASHTABLE_INIT;
    const size_t init_capacity = (1 << JAEGERTRACINGC_HASHTABLE_INIT_ORDER);
    hashtable->buckets =
        jaeger_malloc(init_capacity * sizeof(jaeger_hashtable_bucket));
    memset(hashtable->buckets, 0, sizeof(jaeger_hashtable_bucket));
    if (hashtable->buckets == NULL ||
        !jaeger_hashtable_list_init(&hashtable->list) ||
        !jaeger_hashtable_list_init(&hashtable->ordered_list)) {
        jaeger_hashtable_destroy(hashtable);
        return false;
    }
    hashtable->size = 0;
    hashtable->order = JAEGERTRACINGC_HASHTABLE_INIT_ORDER;
    for (size_t i = 0; i < init_capacity; i++) {
        buckets[i] =
            (jaeger_hashtable_bucket) JAEGERTRACINGC_HASHTABLE_BUCKET_INIT(
                *hashtable);
    }
    return true;
}

#ifdef __cplusplus
} /* extern C */
#endif /* __cplusplus */

#endif /* JAEGERTRACINGC_HASHTABLE_H */
