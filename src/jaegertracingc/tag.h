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
#include "jaegertracingc/protoc-gen/jaeger.pb-c.h"
#include "jaegertracingc/vector.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef Jaegertracing__Protobuf__Tag jaeger_tag;

#define JAEGERTRACINGC_TAG_INIT JAEGERTRACING__PROTOBUF__TAG__INIT

#define JAEGERTRACINGC_TAG_TYPE(type) \
    JAEGERTRACING__PROTOBUF__TAG__VALUE_##type##_VALUE

typedef struct jaeger_tag_list {
    jaeger_vector tags;
} jaeger_tag_list;

static inline bool jaeger_tag_list_init(jaeger_tag_list* list,
                                        jaeger_logger* logger)
{
    assert(list != NULL);
    return jaeger_vector_init(&list->tags, sizeof(jaeger_tag), NULL, logger);
}

static inline bool
jaeger_tag_copy(jaeger_tag* dst, const jaeger_tag* src, jaeger_logger* logger)
{
    assert(dst != NULL);
    assert(src != NULL);
    *dst = (jaeger_tag) JAEGERTRACING__PROTOBUF__TAG__INIT;
    dst->key = jaeger_strdup(src->key, logger);
    if (dst->key == NULL) {
        return false;
    }

    dst->value_case = src->value_case;
    switch (src->value_case) {
    case JAEGERTRACINGC_TAG_TYPE(STR): {
        dst->str_value = jaeger_strdup(src->str_value, logger);
        if (dst->str_value == NULL) {
            jaeger_free(src->key);
            *dst = (jaeger_tag) JAEGERTRACING__PROTOBUF__TAG__INIT;
            return false;
        }
    } break;
    case JAEGERTRACINGC_TAG_TYPE(BINARY): {
        dst->binary_value.data = jaeger_malloc(src->binary_value.len);
        if (dst->binary_value.data == NULL) {
            jaeger_free(src->key);
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

static inline bool
jaeger_tag_copy_wrapper(void* dst, const void* src, jaeger_logger* logger)
{
    return jaeger_tag_copy(dst, src, logger);
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
    JAEGERTRACINGC_VECTOR_FOR_EACH(&list->tags, jaeger_tag_destroy);
    jaeger_vector_clear(&list->tags);
}

static inline bool jaeger_tag_list_append(jaeger_tag_list* list,
                                          const jaeger_tag* tag,
                                          jaeger_logger* logger)
{
    assert(list != NULL);
    assert(tag != NULL);
    jaeger_tag tag_copy;
    if (!jaeger_tag_copy(&tag_copy, tag, logger)) {
        return false;
    }
    jaeger_tag* new_tag = jaeger_vector_append(&list->tags, logger);
    if (new_tag == NULL) {
        jaeger_tag_destroy(&tag_copy);
        return false;
    }
    memcpy(new_tag, &tag_copy, sizeof(tag_copy));
    return true;
}

static inline void jaeger_tag_list_destroy(jaeger_tag_list* list)
{
    if (list != NULL) {
        jaeger_tag_list_clear(list);
        jaeger_vector_destroy(&list->tags);
    }
}

static inline bool jaeger_tag_list_copy(jaeger_tag_list* dst,
                                        const jaeger_tag_list* src,
                                        jaeger_logger* logger)
{
    assert(dst != NULL);
    assert(src != NULL);
    if (jaeger_vector_length(&dst->tags) > 0) {
        jaeger_tag_list_clear(dst);
    }
    if (!jaeger_vector_copy(
            &dst->tags, &src->tags, &jaeger_tag_copy_wrapper, logger)) {
        jaeger_tag_list_clear(dst);
        return false;
    }
    return true;
}

#ifdef __cplusplus
} /* extern C */
#endif /* __cplusplus */

#endif /* JAEGERTRACINGC_TAG_H */
