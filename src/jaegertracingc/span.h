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

#ifndef JAEGERTRACINGC_SPAN_H
#define JAEGERTRACINGC_SPAN_H

#include "jaegertracingc/clock.h"
#include "jaegertracingc/common.h"
#include "jaegertracingc/tag.h"
#include "jaegertracingc/threading.h"
#include "jaegertracingc/trace_id.h"
#include "jaegertracingc/vector.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* 3 delimiters, 2 uint64_t hex (8 characters each), 1 short hex (2 characters).
 * 3 + 16 + 2 = 21 */
#define JAEGERTRACINGC_SPAN_CONTEXT_MAX_STR_LEN \
    JAEGERTRACINGC_TRACE_ID_MAX_STR_LEN + 21

typedef struct jaeger_log_record {
    jaeger_timestamp timestamp;
    jaeger_vector tags;
} jaeger_log_record;

static inline bool jaeger_log_record_init(jaeger_log_record* log_record,
                                          jaeger_logger* logger)
{
    assert(log_record != NULL);
    jaeger_timestamp_now(&log_record->timestamp);
    return jaeger_vector_init(
        &log_record->tags, sizeof(jaeger_tag), NULL, logger);
}

static inline bool jaeger_log_record_copy(jaeger_log_record* dst,
                                          const jaeger_log_record* src,
                                          jaeger_logger* logger)
{
    assert(dst != NULL);
    assert(src != NULL);
    if (!jaeger_vector_copy(
            &dst->tags, &src->tags, &jaeger_tag_copy_wrapper, NULL, logger)) {
        return false;
    }
    dst->timestamp = src->timestamp;
    return true;
}

JAEGERTRACINGC_WRAP_COPY(jaeger_log_record_copy)

static inline void jaeger_log_record_destroy(jaeger_log_record* log_record)
{
    if (log_record != NULL) {
        JAEGERTRACINGC_VECTOR_FOR_EACH(&log_record->tags, jaeger_tag_destroy);
        jaeger_vector_destroy(&log_record->tags);
    }
}

JAEGERTRACINGC_WRAP_DESTROY(jaeger_log_record_destroy)

static inline void
jaeger_log_record_protobuf_destroy(Jaegertracing__Protobuf__Log* log_record)
{
    if (log_record == NULL || log_record->fields == NULL) {
        return;
    }

    for (int i = 0; i < log_record->n_fields; i++) {
        if (log_record->fields[i] != NULL) {
            jaeger_tag_destroy(log_record->fields[i]);
            jaeger_free(log_record->fields[i]);
        }
    }
    jaeger_free(log_record->fields);
    *log_record =
        (Jaegertracing__Protobuf__Log) JAEGERTRACING__PROTOBUF__LOG__INIT;
}

JAEGERTRACINGC_WRAP_DESTROY(jaeger_log_record_protobuf_destroy)

static inline bool
jaeger_log_record_to_protobuf(Jaegertracing__Protobuf__Log* dst,
                              const jaeger_log_record* src,
                              jaeger_logger* logger)
{
    assert(dst != NULL);
    assert(src != NULL);
    dst->has_timestamp = true;
    dst->timestamp = jaeger_timestamp_microseconds(&src->timestamp);
    if (!jaeger_vector_protobuf_copy((void***) &dst->fields,
                                     &dst->n_fields,
                                     &src->tags,
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

JAEGERTRACINGC_WRAP_COPY(jaeger_log_record_to_protobuf)

typedef struct jaeger_span_context {
    jaeger_trace_id trace_id;
    uint64_t span_id;
    uint64_t parent_id;
    uint8_t flags;
} jaeger_span_context;

typedef Jaegertracing__Protobuf__SpanRef__Type jaeger_span_ref_type;

typedef struct jaeger_span_ref {
    jaeger_span_context context;
    jaeger_span_ref_type type;
} jaeger_span_ref;

static inline bool jaeger_span_ref_copy(void* arg,
                                        void* dst,
                                        const void* src,
                                        jaeger_logger* logger)
{
    (void) arg;
    (void) logger;
    assert(dst != NULL);
    assert(src != NULL);
    jaeger_span_ref* dst_span_ref = dst;
    const jaeger_span_ref* src_span_ref = src;
    dst_span_ref->context = src_span_ref->context;
    dst_span_ref->type = src_span_ref->type;
    return true;
}

static inline void
jaeger_span_ref_protobuf_destroy(Jaegertracing__Protobuf__SpanRef* span_ref)
{
    if (span_ref == NULL || span_ref->trace_id == NULL) {
        return;
    }

    jaeger_free(span_ref->trace_id);
    span_ref->trace_id = NULL;
}

JAEGERTRACINGC_WRAP_DESTROY(jaeger_span_ref_protobuf_destroy)

static inline bool
jaeger_span_ref_to_protobuf(Jaegertracing__Protobuf__SpanRef* dst,
                            const jaeger_span_ref* src,
                            jaeger_logger* logger)
{
    assert(dst != NULL);
    assert(src != NULL);
    dst->trace_id = jaeger_malloc(sizeof(Jaegertracing__Protobuf__TraceID));
    if (dst->trace_id == NULL) {
        if (logger != NULL) {
            logger->error(logger,
                          "Cannot allocate trace ID for span ref message");
        }
        return false;
    }
    jaeger_trace_id_to_protobuf(dst->trace_id, &src->context.trace_id);
    dst->has_type = true;
    dst->type = src->type;
    dst->span_id = src->context.span_id;
    return true;
}

JAEGERTRACINGC_WRAP_COPY(jaeger_span_ref_to_protobuf)

typedef struct jaeger_tracer jaeger_tracer;

typedef struct jaeger_span {
    jaeger_tracer* tracer;
    jaeger_span_context context;
    char* operation_name;
    time_t start_time_system;
    jaeger_duration start_time_steady;
    jaeger_duration clock;
    jaeger_vector tags;
    jaeger_vector logs;
    jaeger_vector refs;
    jaeger_mutex mutex;
} jaeger_span;

/* N.B. Caller must be holding span->mutex if span is not null. */
static inline void jaeger_span_clear(jaeger_span* span)
{
    if (span == NULL) {
        return;
    }
    if (span->tracer != NULL) {
        span->tracer = NULL;
    }
    if (span->operation_name != NULL) {
        jaeger_free(span->operation_name);
        span->operation_name = NULL;
    }
    JAEGERTRACINGC_VECTOR_FOR_EACH(&span->tags, jaeger_tag_destroy);
    jaeger_vector_clear(&span->tags);
    JAEGERTRACINGC_VECTOR_FOR_EACH(&span->logs, jaeger_log_record_destroy);
    jaeger_vector_clear(&span->logs);
    jaeger_vector_clear(&span->refs);
}

static inline void jaeger_span_destroy(jaeger_span* span)
{
    if (span == NULL) {
        return;
    }

    jaeger_mutex_lock(&span->mutex);
    jaeger_span_clear(span);
    jaeger_vector_destroy(&span->tags);
    jaeger_vector_destroy(&span->logs);
    jaeger_vector_destroy(&span->refs);
    jaeger_mutex_unlock(&span->mutex);
    jaeger_mutex_destroy(&span->mutex);
}

static inline bool jaeger_span_init(jaeger_span* span, jaeger_logger* logger)
{
    assert(span != NULL);
    memset(span, 0, sizeof(*span));
    if (!jaeger_vector_init(&span->tags, sizeof(jaeger_tag), NULL, logger)) {
        return false;
    }
    if (!jaeger_vector_init(
            &span->logs, sizeof(jaeger_log_record), NULL, logger)) {
        goto cleanup;
    }
    if (!jaeger_vector_init(
            &span->refs, sizeof(jaeger_span_ref), NULL, logger)) {
        goto cleanup;
    }
    span->mutex = (jaeger_mutex) JAEGERTRACINGC_MUTEX_INIT;
    return true;

cleanup:
    jaeger_span_destroy(span);
    return false;
}

static inline bool jaeger_span_copy(jaeger_span* dst,
                                    const jaeger_span* src,
                                    jaeger_logger* logger)
{
    assert(dst != NULL);
    assert(src != NULL);
    jaeger_lock(&dst->mutex, (jaeger_mutex*) &src->mutex);
    dst->context = src->context;
    dst->start_time_system = src->start_time_system;
    dst->start_time_steady = src->start_time_steady;
    dst->clock = src->clock;
    dst->operation_name = jaeger_strdup(src->operation_name, logger);
    if (dst->operation_name == NULL) {
        goto cleanup;
    }
    if (!jaeger_vector_copy(
            &dst->tags, &src->tags, &jaeger_tag_copy_wrapper, NULL, logger)) {
        goto cleanup;
    }

    if (!jaeger_vector_copy(&dst->logs,
                            &src->logs,
                            &jaeger_log_record_to_protobuf_wrapper,
                            &jaeger_log_record_protobuf_destroy_wrapper,
                            logger)) {
        goto cleanup;
    }

    if (!jaeger_vector_copy(
            &dst->refs, &src->refs, &jaeger_span_ref_copy, NULL, logger)) {
        goto cleanup;
    }

    jaeger_mutex_unlock(&dst->mutex);
    jaeger_mutex_unlock((jaeger_mutex*) &src->mutex);
    return true;

cleanup:
    jaeger_mutex_unlock((jaeger_mutex*) &src->mutex);
    jaeger_span_clear(dst);
    jaeger_mutex_unlock(&dst->mutex);
    return false;
}

static inline void
jaeger_span_protobuf_destroy(Jaegertracing__Protobuf__Span* span)
{
    if (span != NULL) {
        if (span->trace_id != NULL) {
            jaeger_free(span->trace_id);
            span->trace_id = NULL;
        }
        if (span->operation_name != NULL) {
            jaeger_free(span->operation_name);
            span->operation_name = NULL;
        }
        if (span->references != NULL) {
            for (int i = 0; i < span->n_references; i++) {
                if (span->references[i] != NULL) {
                    jaeger_span_ref_protobuf_destroy(span->references[i]);
                    jaeger_free(span->references[i]);
                }
            }
            jaeger_free(span->references);
            span->references = NULL;
            span->n_references = 0;
        }
    }
}

static inline bool jaeger_span_to_protobuf(Jaegertracing__Protobuf__Span* dst,
                                           const jaeger_span* src,
                                           jaeger_logger* logger)
{
    assert(dst != NULL);
    assert(src != NULL);
    dst->trace_id = jaeger_malloc(sizeof(Jaegertracing__Protobuf__TraceID));
    if (dst->trace_id == NULL) {
        if (logger != NULL) {
            logger->error(logger, "Cannot allocate trace ID for span message");
        }
        return false;
    }
    dst->has_span_id = true;
    dst->has_parent_span_id = true;
    jaeger_mutex_lock((jaeger_mutex*) &src->mutex);
    jaeger_trace_id_to_protobuf(dst->trace_id, &src->context.trace_id);
    dst->span_id = src->context.span_id;
    dst->parent_span_id = src->context.parent_id;
    dst->operation_name = jaeger_strdup(src->operation_name, logger);
    if (dst->operation_name == NULL) {
        goto cleanup;
    }

    if (!jaeger_vector_protobuf_copy((void***) &dst->tags,
                                     &dst->n_tags,
                                     &src->tags,
                                     sizeof(jaeger_tag),
                                     &jaeger_tag_copy_wrapper,
                                     &jaeger_tag_destroy_wrapper,
                                     NULL,
                                     logger)) {
        goto cleanup;
    }
    if (!jaeger_vector_protobuf_copy(
            (void***) &dst->logs,
            &dst->n_logs,
            &src->logs,
            sizeof(Jaegertracing__Protobuf__Log),
            &jaeger_log_record_to_protobuf_wrapper,
            &jaeger_log_record_protobuf_destroy_wrapper,
            NULL,
            logger)) {
        goto cleanup;
    }
    if (!jaeger_vector_protobuf_copy((void***) &dst->references,
                                     &dst->n_references,
                                     &src->refs,
                                     sizeof(Jaegertracing__Protobuf__SpanRef),
                                     &jaeger_span_ref_to_protobuf_wrapper,
                                     &jaeger_span_ref_protobuf_destroy_wrapper,
                                     NULL,
                                     logger)) {
        goto cleanup;
    }
    jaeger_mutex_unlock((jaeger_mutex*) &src->mutex);
    return true;

cleanup:
    jaeger_mutex_unlock((jaeger_mutex*) &src->mutex);
    jaeger_span_protobuf_destroy(dst);
    *dst = (Jaegertracing__Protobuf__Span) JAEGERTRACING__PROTOBUF__SPAN__INIT;
    return false;
}

static inline int jaeger_span_context_format(const jaeger_span_context* ctx,
                                             char* buffer,
                                             int buffer_len)
{
    assert(ctx != NULL);
    assert(buffer != NULL);
    assert(buffer_len >= 0);
    const int trace_id_len =
        jaeger_trace_id_format(&ctx->trace_id, buffer, buffer_len);
    if (trace_id_len > buffer_len) {
        return trace_id_len + snprintf(NULL,
                                       0,
                                       ":%" PRIx64 ":%" PRIx64 ":%" PRIx8,
                                       ctx->span_id,
                                       ctx->parent_id,
                                       ctx->flags);
    }
    return snprintf(&buffer[trace_id_len],
                    buffer_len - trace_id_len,
                    ":%" PRIx64 ":%" PRIx64 ":%" PRIx8,
                    ctx->span_id,
                    ctx->parent_id,
                    ctx->flags);
}

static inline bool jaeger_span_context_scan(jaeger_span_context* ctx,
                                            const char* str)
{
    assert(ctx != NULL);
    assert(str != NULL);
    jaeger_trace_id trace_id = JAEGERTRACINGC_TRACE_ID_INIT;
    const int len = strlen(str);
    char buffer[len + 1];
    memcpy(buffer, str, len);
    buffer[len] = '\0';
    uint64_t span_id = 0;
    uint64_t parent_id = 0;
    uint8_t flags = 0;
    char* token = buffer;
    char* token_context = NULL;
    char* token_end = NULL;
    for (int i = 0; i < 5; i++) {
        token = strtok_r(i == 0 ? token : NULL, ":", &token_context);
        if (token == NULL && i != 4) {
            return false;
        }
        token_end = NULL;
        switch (i) {
        case 0:
            if (!jaeger_trace_id_scan(&trace_id, token)) {
                return false;
            }
            break;
        case 1:
            span_id = strtoull(token, &token_end, 16);
            break;
        case 2:
            parent_id = strtoull(token, &token_end, 16);
            break;
        case 3:
            flags = strtoull(token, &token_end, 16);
            break;
        default:
            assert(i == 4);
            break;
        }
        if (token_end != NULL && *token_end != '\0') {
            return false;
        }
    }

    *ctx = (jaeger_span_context){.trace_id = trace_id,
                                 .span_id = span_id,
                                 .parent_id = parent_id,
                                 .flags = flags};
    return true;
}

#ifdef __cplusplus
} /* extern C */
#endif /* __cplusplus */

#endif /* JAEGERTRACINGC_SPAN_H */
