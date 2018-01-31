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

#include "jaegertracingc/key_value.h"
#include <assert.h>
#include <stdio.h>
#include "sds.h"
#include "jaegertracingc/alloc.h"

#define INIT_SIZE 10
#define RESIZE_FACTOR 2

#define ALLOC_LIST(list) \
    do { \
    } while (0)

static inline bool alloc_kv(jaeger_key_value_list* list)
{
    list->kv =
        jaeger_global_alloc_malloc(sizeof(jaeger_key_value) * INIT_SIZE);
    if (list->kv == NULL) {
        fprintf(stderr, "ERROR: Cannot allocate jaeger_key_value_list\n");
        return false;
    }
    return true;
}

static inline bool resize(jaeger_key_value_list* list)
{
    const int new_capacity = list->capacity * RESIZE_FACTOR;
    jaeger_key_value* new_kv =
        jaeger_global_alloc_realloc(list->kv, new_capacity);
    if (new_kv == NULL) {
        fprintf(stderr,
                "ERROR: Cannot allocate more space for jaeger_key_value_list,"
                "current capacity = %d, resize factor = %d\n",
                list->capacity,
                RESIZE_FACTOR);
        return false;
    }
    list->capacity = new_capacity;
    list->kv = new_kv;
    return true;
}

bool jaeger_key_value_list_init(jaeger_key_value_list* list)
{
    assert(list != NULL);
    if (!alloc_kv(list)) {
        return false;
    }
    list->size = 0;
    list->capacity = INIT_SIZE;
    return true;
}

bool jaeger_key_value_list_append(
    jaeger_key_value_list* list, const char* key, const char* value)
{
    assert(list != NULL);
    if (list->kv == NULL && !alloc_kv(list)) {
        return false;
    }
    assert(list->size <= list->capacity);
    if (list->size == list->capacity && !resize(list)) {
        return false;
    }
    jaeger_key_value* new_entry = &list->kv[list->size];
    new_entry->key = sdsnew(key);
    new_entry->value = sdsnew(value);
    list->size++;
    return true;
}

void jaeger_key_value_list_free(jaeger_key_value_list* list)
{
    assert(list != NULL);
    if (list->kv != NULL) {
        for (int i = 0; i < list->size; i++) {
            sdsfree(list->kv[i].key);
            sdsfree(list->kv[i].value);
        }
        jaeger_global_alloc_free(list->kv);
    }
}
