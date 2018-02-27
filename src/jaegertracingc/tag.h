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
 * Tag representation and functions.
 */

#ifndef JAEGERTRACINGC_TAG_H
#define JAEGERTRACINGC_TAG_H

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

static inline void jaeger_tag_destroy(jaeger_tag* tag)
{
    if (tag == NULL) {
        return;
    }

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

JAEGERTRACINGC_WRAP_DESTROY(jaeger_tag_destroy, jaeger_tag)

/** Initialize a tag with no value.
 * @param tag The tag instance.
 * @param key The tag key.
 * @param logger Logger to log to in case of error.
 * @return True on success, false otherwise.
 */
static inline bool
jaeger_tag_init(jaeger_tag* tag, const char* key, jaeger_logger* logger)
{
    assert(tag != NULL);
    assert(key != NULL);

    tag->key = jaeger_strdup(key, logger);
    if (tag->key == NULL) {
        return false;
    }
    tag->value_case = JAEGERTRACING__PROTOBUF__TAG__VALUE__NOT_SET;
    return true;
}

static inline bool
jaeger_tag_copy(jaeger_tag* dst, const jaeger_tag* src, jaeger_logger* logger)
{
    assert(dst != NULL);
    assert(src != NULL);
    *dst = (jaeger_tag) JAEGERTRACINGC_TAG_INIT;
    if (!jaeger_tag_init(dst, src->key, logger)) {
        return false;
    }

    dst->value_case = src->value_case;
    switch (src->value_case) {
    case JAEGERTRACINGC_TAG_TYPE(STR): {
        if (src->str_value != NULL) {
            dst->str_value = jaeger_strdup(src->str_value, logger);
            if (dst->str_value == NULL) {
                goto cleanup;
            }
        }
    } break;
    case JAEGERTRACINGC_TAG_TYPE(BINARY): {
        if (src->binary_value.len > 0 && src->binary_value.data != NULL) {
            dst->binary_value.data =
                (uint8_t*) jaeger_malloc(src->binary_value.len);
            if (dst->binary_value.data == NULL) {
                goto cleanup;
            }
            memcpy(dst->binary_value.data,
                   src->binary_value.data,
                   src->binary_value.len);
        }
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

cleanup:
    jaeger_tag_destroy(dst);
    *dst = (jaeger_tag) JAEGERTRACING__PROTOBUF__TAG__INIT;
    return false;
}

JAEGERTRACINGC_WRAP_COPY(jaeger_tag_copy, jaeger_tag, jaeger_tag)

static inline bool jaeger_tag_vector_append(jaeger_vector* vec,
                                            const jaeger_tag* tag,
                                            jaeger_logger* logger)
{
    jaeger_tag* tag_copy = (jaeger_tag*) jaeger_vector_append(vec, logger);
    if (tag_copy == NULL) {
        return false;
    }
    if (!jaeger_tag_copy(tag_copy, tag, logger)) {
        vec->len--;
        return false;
    }
    return true;
}

#ifdef __cplusplus
} /* extern C */
#endif /* __cplusplus */

#endif /* JAEGERTRACINGC_TAG_H */
