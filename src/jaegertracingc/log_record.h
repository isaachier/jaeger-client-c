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

static inline void jaeger_log_record_destroy(jaeger_log_record* log_record)
{
    if (log_record == NULL) {
        return;
    }
    JAEGERTRACINGC_VECTOR_FOR_EACH(
        &log_record->fields, jaeger_tag_destroy, jaeger_tag);
    jaeger_vector_destroy(&log_record->fields);
}

JAEGERTRACINGC_WRAP_DESTROY(jaeger_log_record_destroy, jaeger_log_record)

#define JAEGERTRACINGC_LOG_RECORD_INIT              \
    {                                               \
        .timestamp = JAEGERTRACINGC_TIMESTAMP_INIT, \
        .fields = JAEGERTRACINGC_VECTOR_INIT        \
    }

static inline bool jaeger_log_record_init(jaeger_log_record* log_record,
                                          jaeger_logger* logger)
{
    assert(log_record != NULL);
    jaeger_timestamp_now(&log_record->timestamp);
    return jaeger_vector_init(
        &log_record->fields, sizeof(jaeger_tag), NULL, logger);
}

static inline bool jaeger_log_record_copy(jaeger_log_record* restrict dst,
                                          const jaeger_log_record* restrict src,
                                          jaeger_logger* logger)
{
    assert(dst != NULL);
    assert(src != NULL);
    if (!jaeger_log_record_init(dst, logger)) {
        return false;
    }
    if (!jaeger_vector_copy(&dst->fields,
                            &src->fields,
                            &jaeger_tag_copy_wrapper,
                            NULL,
                            logger)) {
        jaeger_log_record_destroy(dst);
        return false;
    }
    dst->timestamp = src->timestamp;
    return true;
}

JAEGERTRACINGC_WRAP_COPY(jaeger_log_record_copy,
                         jaeger_log_record,
                         jaeger_log_record)

static inline void
jaeger_log_record_protobuf_destroy(Jaegertracing__Protobuf__Log* log_record)
{
    if (log_record == NULL || log_record->fields == NULL) {
        return;
    }

    for (int i = 0; i < (int) log_record->n_fields; i++) {
        if (log_record->fields[i] != NULL) {
            jaeger_tag_destroy(log_record->fields[i]);
            jaeger_free(log_record->fields[i]);
        }
    }
    jaeger_free(log_record->fields);
    *log_record =
        (Jaegertracing__Protobuf__Log) JAEGERTRACING__PROTOBUF__LOG__INIT;
}

JAEGERTRACINGC_WRAP_DESTROY(jaeger_log_record_protobuf_destroy,
                            Jaegertracing__Protobuf__Log)

static inline bool
jaeger_log_record_to_protobuf(Jaegertracing__Protobuf__Log* restrict dst,
                              const jaeger_log_record* restrict src,
                              jaeger_logger* logger)
{
    assert(dst != NULL);
    assert(src != NULL);
    *dst = (Jaegertracing__Protobuf__Log) JAEGERTRACING__PROTOBUF__LOG__INIT;
#ifdef JAEGERTRACINGC_HAVE_PROTOBUF_OPTIONAL_FIELDS
    dst->has_timestamp = true;
#endif /* JAEGERTRACINGC_HAVE_PROTOBUF_OPTIONAL_FIELDS */
    dst->timestamp = jaeger_timestamp_microseconds(&src->timestamp);
    if (!jaeger_vector_protobuf_copy((void***) &dst->fields,
                                     &dst->n_fields,
                                     &src->fields,
                                     sizeof(jaeger_tag),
                                     &jaeger_tag_copy_wrapper,
                                     &jaeger_tag_destroy_wrapper,
                                     NULL,
                                     logger)) {
        jaeger_log_record_protobuf_destroy(dst);
        *dst =
            (Jaegertracing__Protobuf__Log) JAEGERTRACING__PROTOBUF__LOG__INIT;
        return false;
    }
    return true;
}

JAEGERTRACINGC_WRAP_COPY(jaeger_log_record_to_protobuf,
                         Jaegertracing__Protobuf__Log,
                         jaeger_log_record)

#ifdef __cplusplus
} /* extern C */
#endif /* __cplusplus */

#endif /* JAEGERTRACINGC_LOG_RECORD_H */
