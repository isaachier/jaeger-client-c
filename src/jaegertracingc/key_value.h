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

#ifndef JAEGERTRACINGC_KEY_VALUE_H
#define JAEGERTRACINGC_KEY_VALUE_H

#include "jaegertracingc/alloc.h"
#include "jaegertracingc/common.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

#define JAEGERTRACINGC_KV_INIT_SIZE 10
#define JAEGERTRACINGC_KV_RESIZE_FACTOR 2

typedef struct jaeger_key_value
{
    char* key;
    char* value;
} jaeger_key_value;

typedef struct jaeger_key_value_list
{
    jaeger_key_value* kv;
    int size;
    int capacity;
} jaeger_key_value_list;

static inline bool jaeger_key_value_alloc_list(jaeger_key_value_list* list)
{
    list->kv =
        jaeger_malloc(sizeof(jaeger_key_value) * JAEGERTRACINGC_KV_INIT_SIZE);
    if (list->kv == NULL)
    {
        fprintf(stderr, "ERROR: Cannot allocate jaeger_key_value_list\n");
        return false;
    }
    list->capacity = JAEGERTRACINGC_KV_INIT_SIZE;
    return true;
}

static inline bool jaeger_key_value_resize(jaeger_key_value_list* list)
{
    assert(list != NULL);
    const int new_capacity = list->capacity * JAEGERTRACINGC_KV_RESIZE_FACTOR;
    jaeger_key_value* new_kv =
        jaeger_realloc(list->kv, sizeof(jaeger_key_value) * new_capacity);
    if (new_kv == NULL)
    {
        fprintf(stderr,
                "ERROR: Cannot allocate more space for jaeger_key_value_list,"
                "current capacity = %d, resize factor = %d\n",
                list->capacity,
                JAEGERTRACINGC_KV_RESIZE_FACTOR);
        return false;
    }
    list->capacity = new_capacity;
    list->kv = new_kv;
    return true;
}

static inline bool jaeger_key_value_list_init(jaeger_key_value_list* list)
{
    assert(list != NULL);
    if (!jaeger_key_value_alloc_list(list))
    {
        return false;
    }
    list->size = 0;
    list->capacity = JAEGERTRACINGC_KV_INIT_SIZE;
    return true;
}

static inline bool jaeger_key_value_list_append(jaeger_key_value_list* list,
                                                const char* key,
                                                const char* value)
{
    assert(list != NULL);
    if (list->kv == NULL && !jaeger_key_value_alloc_list(list))
    {
        return false;
    }
    assert(list->size <= list->capacity);
    if (list->size == list->capacity && !jaeger_key_value_resize(list))
    {
        return false;
    }
    const int key_size = strlen(key);
    char* key_copy = jaeger_malloc(key_size);
    if (key_copy == NULL)
    {
        fprintf(stderr, "ERROR: Cannot allocate key, size=%d\n", key_size);
        return false;
    }
    memcpy(key_copy, key, key_size);
    const int value_size = strlen(value);
    char* value_copy = jaeger_malloc(value_size);
    if (value_copy == NULL)
    {
        fprintf(stderr, "ERROR: Cannot allocate value, size=%d\n", value_size);
        return false;
    }
    memcpy(value_copy, value, value_size);
    list->kv[list->size] =
        (jaeger_key_value){.key = key_copy, .value = value_copy};
    list->size++;
    return true;
}

static inline void jaeger_key_value_list_free(jaeger_key_value_list* list)
{
    assert(list != NULL);
    if (list->kv != NULL)
    {
        for (int i = 0; i < list->size; i++)
        {
            jaeger_free(list->kv[i].key);
            jaeger_free(list->kv[i].value);
        }
        jaeger_free(list->kv);
    }
}

#endif // JAEGERTRACINGC_KEY_VALUE_H
