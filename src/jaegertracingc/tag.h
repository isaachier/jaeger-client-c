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

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "jaegertracingc/alloc.h"
#include "jaegertracingc/common.h"
#include "jaegertracingc/types.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define JAEGERTRACINGC_TAGS_INIT_SIZE 10
#define JAEGERTRACINGC_TAGS_RESIZE_FACTOR 2

typedef struct jaeger_tag_list {
    jaeger_tag* tags;
    int size;
    int capacity;
} jaeger_tag_list;

static inline bool jaeger_tag_alloc_list(jaeger_tag_list* list,
                                         jaeger_logger* logger)
{
    assert(logger != NULL);
    list->tags =
        jaeger_malloc(sizeof(jaeger_tag) * JAEGERTRACINGC_TAGS_INIT_SIZE);
    if (list->tags == NULL) {
        logger->error(logger, "Cannot allocate jaeger_tag_list");
        return false;
    }
    list->capacity = JAEGERTRACINGC_TAGS_INIT_SIZE;
    return true;
}

static inline bool jaeger_tag_resize(jaeger_tag_list* list,
                                     jaeger_logger* logger)
{
    assert(list != NULL);
    const int new_capacity = list->capacity * JAEGERTRACINGC_TAGS_RESIZE_FACTOR;
    jaeger_tag* new_tags =
        jaeger_realloc(list->tags, sizeof(jaeger_tag) * new_capacity);
    if (new_tags == NULL) {
        logger->error(logger,
                      "Cannot allocate more space for jaeger_tag_list,"
                      "current capacity = %d, resize factor = %d",
                      list->capacity,
                      JAEGERTRACINGC_TAGS_RESIZE_FACTOR);
        return false;
    }
    list->capacity = new_capacity;
    list->tags = new_tags;
    return true;
}

static inline bool jaeger_tag_list_init(jaeger_tag_list* list,
                                        jaeger_logger* logger)
{
    assert(list != NULL);
    if (!jaeger_tag_alloc_list(list, logger)) {
        return false;
    }
    list->size = 0;
    list->capacity = JAEGERTRACINGC_TAGS_INIT_SIZE;
    return true;
}

static inline bool
jaeger_tag_copy(jaeger_tag* dst, const jaeger_tag* src, jaeger_logger* logger)
{
    assert(dst != NULL);
    assert(src != NULL);
    *dst = (jaeger_tag) JAEGERTRACING__PROTOBUF__TAG__INIT;
    dst->key = jaeger_strdup(src->key, logger);
    if (dst->key == NULL) {
        logger->error(
            logger, "Cannot allocate tag key, key = \"%s\"", src->key);
        *dst = (jaeger_tag) JAEGERTRACING__PROTOBUF__TAG__INIT;
        return false;
    }

    dst->value_case = src->value_case;
    switch (src->value_case) {
    case JAEGERTRACINGC_TAG_TYPE(STR): {
        dst->str_value = jaeger_strdup(src->str_value, logger);
        if (dst->str_value == NULL) {
            logger->error(logger,
                          "Cannot allocate tag value string, str = \"%s\"",
                          src->str_value);
            *dst = (jaeger_tag) JAEGERTRACING__PROTOBUF__TAG__INIT;
            return false;
        }
    } break;
    case JAEGERTRACINGC_TAG_TYPE(BINARY): {
        dst->binary_value.data = jaeger_malloc(src->binary_value.len);
        if (dst->binary_value.data == NULL) {
            logger->error(logger,
                          "Cannot allocate tag value binary, size = %zu",
                          src->binary_value.len);
            *dst = (jaeger_tag) JAEGERTRACING__PROTOBUF__TAG__INIT;
            return false;
        }
        memcpy(dst->binary_value.data,
               src->binary_value.data,
               src->binary_value.len);
    } break;
    case JAEGERTRACINGC_TAG_TYPE(DOUBLE): {
        dst->double_value = src->double_value;
    } break;
    case JAEGERTRACINGC_TAG_TYPE(BOOL): {
        dst->bool_value = src->bool_value;
    } break;
    default: {
        assert(dst->value_case == JAEGERTRACINGC_TAG_TYPE(LONG));
        dst->long_value = src->long_value;
    } break;
    }
    return true;
}

static inline void jaeger_tag_destroy(jaeger_tag* tag)
{
    if (tag->key != NULL) {
        jaeger_free(tag->key);
        tag->key = NULL;
    }

    switch (tag->value_case) {
    case JAEGERTRACINGC_TAG_TYPE(STR): {
        if (tag->str_value != NULL) {
            jaeger_free(tag->str_value);
            tag->str_value = NULL;
        }
    } break;
    case JAEGERTRACINGC_TAG_TYPE(BINARY): {
        if (tag->binary_value.data != NULL) {
            jaeger_free(tag->binary_value.data);
            tag->binary_value.data = NULL;
        }
    } break;
    default:
        break;
    }
}

static inline void jaeger_tag_list_clear(jaeger_tag_list* list)
{
    assert(list != NULL);
    for (int i = 0; i < list->size; i++) {
        jaeger_tag_destroy(&list->tags[i]);
    }
    list->size = 0;
}

static inline bool jaeger_tag_list_append(jaeger_tag_list* list,
                                          const jaeger_tag* tag,
                                          jaeger_logger* logger)
{
    assert(list != NULL);
    assert(tag != NULL);
    if (list->tags == NULL && !jaeger_tag_alloc_list(list, logger)) {
        return false;
    }
    assert(list->size <= list->capacity);
    if (list->size == list->capacity && !jaeger_tag_resize(list, logger)) {
        return false;
    }

    if (!jaeger_tag_copy(&list->tags[list->size], tag, logger)) {
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
        list->tags = NULL;
    }
}

#ifdef __cplusplus
} /* extern C */
#endif /* __cplusplus */

#endif /* JAEGERTRACINGC_TAG_H */
