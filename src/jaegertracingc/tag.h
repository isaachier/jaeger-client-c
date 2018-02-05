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

#ifndef JAEGERTRACINGC_TAG_H
#define JAEGERTRACINGC_TAG_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "jaegertracingc/alloc.h"
#include "jaegertracingc/common.h"
#include "jaegertracingc/types.h"

#define JAEGERTRACINGC_TAGS_INIT_SIZE 10
#define JAEGERTRACINGC_TAGS_RESIZE_FACTOR 2

typedef struct jaeger_tag_list {
    jaeger_tag* tags;
    int size;
    int capacity;
} jaeger_tag_list;

static inline bool jaeger_tag_alloc_list(jaeger_tag_list* list)
{
    list->tags =
        jaeger_malloc(sizeof(jaeger_tag) * JAEGERTRACINGC_TAGS_INIT_SIZE);
    if (list->tags == NULL) {
        fprintf(stderr, "ERROR: Cannot allocate jaeger_tag_list\n");
        return false;
    }
    list->capacity = JAEGERTRACINGC_TAGS_INIT_SIZE;
    return true;
}

static inline bool jaeger_tag_resize(jaeger_tag_list* list)
{
    assert(list != NULL);
    const int new_capacity = list->capacity * JAEGERTRACINGC_TAGS_RESIZE_FACTOR;
    jaeger_tag* new_tags =
        jaeger_realloc(list->tags, sizeof(jaeger_tag) * new_capacity);
    if (new_tags == NULL) {
        fprintf(stderr,
                "ERROR: Cannot allocate more space for jaeger_tag_list,"
                "current capacity = %d, resize factor = %d\n",
                list->capacity,
                JAEGERTRACINGC_TAGS_RESIZE_FACTOR);
        return false;
    }
    list->capacity = new_capacity;
    list->tags = new_tags;
    return true;
}

static inline bool jaeger_tag_list_init(jaeger_tag_list* list)
{
    assert(list != NULL);
    if (!jaeger_tag_alloc_list(list)) {
        return false;
    }
    list->size = 0;
    list->capacity = JAEGERTRACINGC_TAGS_INIT_SIZE;
    return true;
}

static inline bool jaeger_tag_copy(jaeger_tag* dst, const jaeger_tag* src)
{
    assert(dst != NULL);
    assert(src != NULL);
    *dst = (jaeger_tag) JAEGERTRACING__PROTOBUF__TAG__INIT;
    dst->key = jaeger_strdup(src->key);
    if (dst->key == NULL) {
        fprintf(
            stderr, "ERROR: Cannot allocate tag key, key = \"%s\"\n", src->key);
        *dst = (jaeger_tag) JAEGERTRACING__PROTOBUF__TAG__INIT;
        return false;
    }

    dst->value_case = src->value_case;
    switch (src->value_case) {
    case JAEGERTRACING__PROTOBUF__TAG__VALUE_STR_VALUE: {
        dst->str_value = jaeger_strdup(src->str_value);
        if (dst->str_value == NULL) {
            fprintf(stderr,
                    "ERROR: Cannot allocate tag value string, str = \"%s\"\n",
                    src->str_value);
            *dst = (jaeger_tag) JAEGERTRACING__PROTOBUF__TAG__INIT;
            return false;
        }
    } break;
    case JAEGERTRACING__PROTOBUF__TAG__VALUE_BINARY_VALUE: {
        dst->binary_value.data = jaeger_malloc(src->binary_value.len);
        if (dst->binary_value.data == NULL) {
            fprintf(stderr,
                    "ERROR: Cannot allocate tag value binary, size = %zu\n",
                    src->binary_value.len);
            *dst = (jaeger_tag) JAEGERTRACING__PROTOBUF__TAG__INIT;
            return false;
        }
        memcpy(dst->binary_value.data,
               src->binary_value.data,
               src->binary_value.len);
    } break;
    case JAEGERTRACING__PROTOBUF__TAG__VALUE_DOUBLE_VALUE: {
        dst->double_value = src->double_value;
    } break;
    case JAEGERTRACING__PROTOBUF__TAG__VALUE_BOOL_VALUE: {
        dst->bool_value = src->bool_value;
    } break;
    default: {
        assert(dst->value_case ==
               JAEGERTRACING__PROTOBUF__TAG__VALUE_LONG_VALUE);
        dst->long_value = src->long_value;
    } break;
    }
    return true;
}

static inline void jaeger_tag_destroy(jaeger_tag* tag)
{
    if (tag->key != NULL) {
        jaeger_free(tag->key);
    }

    switch (tag->value_case) {
    case JAEGERTRACING__PROTOBUF__TAG__VALUE_STR_VALUE: {
        if (tag->str_value != NULL) {
            jaeger_free(tag->str_value);
        }
    } break;
    case JAEGERTRACING__PROTOBUF__TAG__VALUE_BINARY_VALUE: {
        if (tag->binary_value.data != NULL) {
            jaeger_free(tag->binary_value.data);
        }
    } break;
    default:
        break;
    }
}

static inline bool jaeger_tag_list_append(jaeger_tag_list* list,
                                          const jaeger_tag* tag)
{
    assert(list != NULL);
    assert(tag != NULL);
    if (list->tags == NULL && !jaeger_tag_alloc_list(list)) {
        return false;
    }
    assert(list->size <= list->capacity);
    if (list->size == list->capacity && !jaeger_tag_resize(list)) {
        return false;
    }

    if (!jaeger_tag_copy(&list->tags[list->size], tag)) {
        return false;
    }
    list->size++;
    return true;
}

static inline void jaeger_tag_list_destroy(jaeger_tag_list* list)
{
    assert(list != NULL);
    if (list->tags != NULL) {
        for (int i = 0; i < list->size; i++) {
            jaeger_tag_destroy(&list->tags[i]);
        }
        jaeger_free(list->tags);
    }
}

#ifdef __cplusplus
} /* extern C */
#endif /* __cplusplus */

#endif /* JAEGERTRACINGC_TAG_H */
