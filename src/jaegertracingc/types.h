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

#ifndef JAEGERTRACINGC_TYPES_H
#define JAEGERTRACINGC_TYPES_H

#include "jaegertracingc/common.h"
#include "jaegertracingc/protoc-gen/jaeger.pb-c.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef Jaegertracing__Protobuf__Log jaeger_log;

#define JAEGERTRACINGC_LOG_INIT JAEGERTRACING__PROTOBUF__LOG__INIT

typedef Jaegertracing__Protobuf__Tag jaeger_tag;

#define JAEGERTRACINGC_TAG_INIT JAEGERTRACING__PROTOBUF__TAG__INIT

#define JAEGERTRACINGC_TAG_TYPE(type) \
    JAEGERTRACING__PROTOBUF__TAG__VALUE_##type##_VALUE

#ifdef __cplusplus
} /* extern C */
#endif /* __cplusplus */

#endif /* JAEGERTRACINGC_TYPES_H */
