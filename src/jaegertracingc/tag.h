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
#include "jaegertracingc/protoc-gen/model.pb-c.h"
#include "jaegertracingc/vector.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef Jaeger__Model__KeyValue jaeger_tag;

#define JAEGERTRACINGC_TAG_INIT JAEGER__MODEL__KEY_VALUE__INIT

#define JAEGERTRACINGC_TAG_TYPE(type) \
    JAEGERTRACING__PROTOBUF__TAG__VALUE_##type##_VALUE

void jaeger_tag_destroy(jaeger_tag* tag);

JAEGERTRACINGC_WRAP_DESTROY(jaeger_tag_destroy, jaeger_tag)

/** Initialize a tag with no value.
 * @param tag The tag instance.
 * @param key The tag key.
 * @return True on success, false otherwise.
 */
bool jaeger_tag_init(jaeger_tag* tag, const char* key);

bool jaeger_tag_copy(jaeger_tag* dst, const jaeger_tag* src);

bool jaeger_tag_from_key_value(jaeger_tag* restrict dst,
                               const char* key,
                               const opentracing_value* value);

JAEGERTRACINGC_WRAP_COPY(jaeger_tag_copy, jaeger_tag, jaeger_tag)

bool jaeger_tag_vector_append(jaeger_vector* vec, const jaeger_tag* tag);

#ifdef __cplusplus
} /* extern C */
#endif /* __cplusplus */

#endif /* JAEGERTRACINGC_TAG_H */
