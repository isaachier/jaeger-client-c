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
 * Log record representation for use in span.
 * @see jaeger_span
 * @see jaeger_span_log
 */

#ifndef JAEGERTRACINGC_LOG_RECORD_H
#define JAEGERTRACINGC_LOG_RECORD_H

#include "jaegertracingc/clock.h"
#include "jaegertracingc/common.h"
#include "jaegertracingc/tag.h"
#include "jaegertracingc/vector.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct jaeger_log_record {
    jaeger_timestamp timestamp;
    jaeger_vector fields;
} jaeger_log_record;

void jaeger_log_record_destroy(jaeger_log_record* log_record);

JAEGERTRACINGC_WRAP_DESTROY(jaeger_log_record_destroy, jaeger_log_record)

#define JAEGERTRACINGC_LOG_RECORD_INIT              \
    {                                               \
        .timestamp = JAEGERTRACINGC_TIMESTAMP_INIT, \
        .fields = JAEGERTRACINGC_VECTOR_INIT        \
    }

bool jaeger_log_record_init(jaeger_log_record* log_record);

bool jaeger_log_record_copy(jaeger_log_record* restrict dst,
                            const jaeger_log_record* restrict src);

bool jaeger_log_record_from_opentracing(jaeger_log_record* restrict dst,
                                        const opentracing_log_record* restrict
                                            src);

JAEGERTRACINGC_WRAP_COPY(jaeger_log_record_copy,
                         jaeger_log_record,
                         jaeger_log_record)

void jaeger_log_record_protobuf_destroy(Jaeger__Model__Log* log_record);

JAEGERTRACINGC_WRAP_DESTROY(jaeger_log_record_protobuf_destroy,
                            Jaeger__Model__Log)

bool jaeger_log_record_to_protobuf(Jaeger__Model__Log* restrict dst,
                                   const jaeger_log_record* restrict src);

JAEGERTRACINGC_WRAP_COPY(jaeger_log_record_to_protobuf,
                         Jaeger__Model__Log,
                         jaeger_log_record)

#ifdef __cplusplus
} /* extern C */
#endif /* __cplusplus */

#endif /* JAEGERTRACINGC_LOG_RECORD_H */
