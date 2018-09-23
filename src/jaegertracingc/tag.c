/*
 * Copyright (c) 2018 The Jaeger Authors.
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

#include "jaegertracingc/tag.h"

void jaeger_tag_destroy(jaeger_tag* tag)
{
    if (tag == NULL) {
        return;
    }

    if (tag->key != NULL) {
        jaeger_free(tag->key);
        tag->key = NULL;
    }

    switch (tag->v_type) {
    case JAEGER__MODEL__VALUE_TYPE__STRING: {
        if (tag->v_str != NULL) {
            jaeger_free(tag->v_str);
            tag->v_str = NULL;
        }
    } break;
    case JAEGER__MODEL__VALUE_TYPE__BINARY: {
        if (tag->v_binary.data != NULL) {
            jaeger_free(tag->v_binary.data);
            tag->v_binary.data = NULL;
        }
    } break;
    default:
        break;
    }
}

bool jaeger_tag_init(jaeger_tag* tag, const char* key)
{
    assert(tag != NULL);
    assert(key != NULL);

    tag->key = jaeger_strdup(key);
    if (tag->key == NULL) {
        return false;
    }
    return true;
}

bool jaeger_tag_copy(jaeger_tag* dst, const jaeger_tag* src)
{
    assert(dst != NULL);
    assert(src != NULL);
    *dst = (jaeger_tag) JAEGERTRACINGC_TAG_INIT;
    if (!jaeger_tag_init(dst, src->key)) {
        return false;
    }

    dst->v_type = src->v_type;
    switch (src->v_type) {
    case JAEGER__MODEL__VALUE_TYPE__STRING: {
        if (src->v_str != NULL) {
            dst->v_str = jaeger_strdup(src->v_str);
            if (dst->v_str == NULL) {
                goto cleanup;
            }
        }
    } break;
    case JAEGER__MODEL__VALUE_TYPE__BINARY: {
        if (src->v_binary.len > 0 && src->v_binary.data != NULL) {
            dst->v_binary.data = (uint8_t*) jaeger_malloc(src->v_binary.len);
            if (dst->v_binary.data == NULL) {
                goto cleanup;
            }
            memcpy(dst->v_binary.data, src->v_binary.data, src->v_binary.len);
        }
    } break;
    case JAEGER__MODEL__VALUE_TYPE__FLOAT64: {
        dst->v_float64 = src->v_float64;
    } break;
    case JAEGER__MODEL__VALUE_TYPE__BOOL: {
        dst->v_bool = src->v_bool;
    } break;
    default: {
        assert(dst->v_type == JAEGER__MODEL__VALUE_TYPE__INT64);
        dst->v_int64 = src->v_int64;
    } break;
    }

    return true;

cleanup:
    jaeger_tag_destroy(dst);
    *dst = (jaeger_tag) JAEGERTRACINGC_TAG_INIT;
    return false;
}

bool jaeger_tag_from_key_value(jaeger_tag* restrict dst,
                               const char* key,
                               const opentracing_value* value)
{
    jaeger_tag src = JAEGERTRACINGC_TAG_INIT;
    char key_copy[strlen(key) + 1];
    strncpy(key_copy, key, sizeof(key_copy));
    src.key = key_copy;
    if (src.key == NULL) {
        return false;
    }
    switch (value->type) {
    case opentracing_value_null:
        break;
    case opentracing_value_bool:
        src.v_type = JAEGER__MODEL__VALUE_TYPE__BOOL;
        src.v_bool = (value->value.bool_value == opentracing_true);
        break;
    case opentracing_value_uint64:
        src.v_type = JAEGER__MODEL__VALUE_TYPE__INT64;
        src.v_int64 = value->value.uint64_value;
        break;
    case opentracing_value_int64:
        src.v_type = JAEGER__MODEL__VALUE_TYPE__INT64;
        src.v_int64 = value->value.int64_value;
        break;
    case opentracing_value_double:
        src.v_type = JAEGER__MODEL__VALUE_TYPE__FLOAT64;
        src.v_float64 = value->value.double_value;
        break;
    case opentracing_value_string:
        src.v_type = JAEGER__MODEL__VALUE_TYPE__STRING;
        src.v_str = jaeger_strdup(value->value.string_value);
        if (src.v_str == NULL) {
            return false;
        }
        break;
    default:
        return false;
    }

    return jaeger_tag_copy(dst, &src);
}

bool jaeger_tag_vector_append(jaeger_vector* vec, const jaeger_tag* tag)
{
    jaeger_tag* tag_copy = (jaeger_tag*) jaeger_vector_append(vec);
    if (tag_copy == NULL) {
        return false;
    }
    if (!jaeger_tag_copy(tag_copy, tag)) {
        vec->len--;
        return false;
    }
    return true;
}
