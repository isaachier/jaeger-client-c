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
 * Span, span context, and span ref representations and functions.
 */

#ifndef JAEGERTRACINGC_SPAN_H
#define JAEGERTRACINGC_SPAN_H

#include "jaegertracingc/clock.h"
#include "jaegertracingc/common.h"
#include "jaegertracingc/key_value.h"
#include "jaegertracingc/log_record.h"
#include "jaegertracingc/tag.h"
#include "jaegertracingc/threading.h"
#include "jaegertracingc/trace_id.h"
#include "jaegertracingc/vector.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * Max number of characters needed for string representation of
 * jaeger_span_context (excluding null byte).
 * 3 delimiters, 2 uint64_t hex (8 characters each), 1 short hex (2 characters).
 * 3 + 16 + 2 = 21
 */
#define JAEGERTRACINGC_SPAN_CONTEXT_MAX_STR_LEN \
    JAEGERTRACINGC_TRACE_ID_MAX_STR_LEN + 21

#define JAEGERTRACINGC_SAMPLING_PRIORITY "sampling.priority"

enum {
    jaeger_sampling_flag_sampled = 1,
    jaeger_sampling_flag_debug = (1 << 1)
};

/**
 * Span context represents propagated span identity and state.
 */
typedef struct jaeger_span_context {
    /** Trace ID of the trace containing this span. */
    jaeger_trace_id trace_id;

    /**
     * Randomly generated unique ID. Must be unique within its trace, but not
     * necessarily globally unique.
     */
    uint64_t span_id;

    /** Span ID of parent span. Parent ID of zero represents root span. */
    uint64_t parent_id;

    /** Sampling flags. */
    uint8_t flags;

    /** Key-value pairs that are propagated along with the span. */
    jaeger_vector baggage;

    /**
     * Can be set to correlation ID when the context is being extracted from a
     * carrier.
     * @see JAEGERTRACINGC_DEBUG_HEADER
     */
    char* debug_id;
} jaeger_span_context;

#define JAEGERTRACINGC_SPAN_CONTEXT_INIT                                   \
    {                                                                      \
        .trace_id = JAEGERTRACINGC_TRACE_ID_INIT, .span_id = 0,            \
        .parent_id = 0, .flags = 0, .baggage = JAEGERTRACINGC_VECTOR_INIT, \
        .debug_id = NULL                                                   \
    }

static inline void jaeger_span_context_destroy(jaeger_span_context* ctx)
{
    if (ctx == NULL) {
        return;
    }
    JAEGERTRACINGC_VECTOR_FOR_EACH(
        &ctx->baggage, jaeger_key_value_destroy, jaeger_key_value);
    jaeger_vector_destroy(&ctx->baggage);
    if (ctx->debug_id != NULL) {
        jaeger_free(ctx->debug_id);
        ctx->debug_id = NULL;
    }
}

static inline bool jaeger_span_context_init(jaeger_span_context* ctx)
{
    assert(ctx != NULL);
    *ctx = (jaeger_span_context) JAEGERTRACINGC_SPAN_CONTEXT_INIT;
    return jaeger_vector_init(&ctx->baggage, sizeof(jaeger_key_value));
}

static inline bool
jaeger_span_context_copy(jaeger_span_context* restrict dst,
                         const jaeger_span_context* restrict src)
{
    assert(dst != NULL);
    assert(src != NULL);
    if (!jaeger_span_context_init(dst)) {
        return false;
    }
    if (!jaeger_vector_copy(&dst->baggage,
                            &src->baggage,
                            &jaeger_key_value_copy_wrapper,
                            NULL)) {
        jaeger_span_context_destroy(dst);
        return false;
    }
    dst->trace_id = src->trace_id;
    dst->span_id = src->span_id;
    dst->parent_id = src->parent_id;
    dst->flags = src->flags;
    return true;
}

/**
 * @internal
 * Returns whether or not the span context is valid.
 * @param ctx Span context instance. May not be NULL.
 * @return True if valid, false otherwise.
 */
static inline bool jaeger_span_context_is_valid(const jaeger_span_context* ctx)
{
    assert(ctx != NULL);
    return ctx->trace_id.high != 0 || ctx->trace_id.low != 0;
}

/**
 * @internal
 * Determines whether or not a span context is only used to return the
 * debug/correlation ID from extract() method. This happens in the situation
 * when "jaeger-debug-id" header is passed in the carrier to the extract()
 * method, but the request otherwise has no span context in it. Previously
 * this
 * would have returned an error (opentracing.ErrSpanContextNotFound) from
 * the
 * extract method, but now it returns a dummy context with only debugID
 * filled
 * in.
 * @param ctx Span context instance. May not be NULL.
 * @return True if just debug container, false otherwise.
 */
static inline bool
jaeger_span_context_is_debug_id_container_only(const jaeger_span_context* ctx)
{
    assert(ctx != NULL);
    return !jaeger_span_context_is_valid(ctx) && ctx->debug_id != NULL;
}

typedef Jaegertracing__Protobuf__SpanRef__Type jaeger_span_ref_type;

typedef struct jaeger_span_ref {
    jaeger_span_context context;
    jaeger_span_ref_type type;
} jaeger_span_ref;

#define JAEGERTRACINGC_SPAN_REF_INIT                 \
    {                                                \
        .context = JAEGERTRACINGC_SPAN_CONTEXT_INIT, \
        .type = (jaeger_span_ref_type) -1            \
    }

static inline bool jaeger_span_ref_init(jaeger_span_ref* span_ref)
{
    assert(span_ref != NULL);
    *span_ref = (jaeger_span_ref) JAEGERTRACINGC_SPAN_REF_INIT;
    return jaeger_span_context_init(&span_ref->context);
}

static inline void jaeger_span_ref_destroy(jaeger_span_ref* span_ref)
{
    if (span_ref == NULL) {
        return;
    }

    jaeger_span_context_destroy(&span_ref->context);
}

static inline bool
jaeger_span_ref_copy(void* arg, void* restrict dst, const void* restrict src)
{
    (void) arg;
    assert(dst != NULL);
    assert(src != NULL);
    jaeger_span_ref* dst_span_ref = (jaeger_span_ref*) dst;
    /* Do not call jaeger_span_ref_init so we avoid memory leak in
     * jaeger_span_context_copy due to double initialization of span
     * context.
     */
    const jaeger_span_ref* src_span_ref = (const jaeger_span_ref*) src;
    if (!jaeger_span_context_copy(&dst_span_ref->context,
                                  &src_span_ref->context)) {
        return false;
    }
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

JAEGERTRACINGC_WRAP_DESTROY(jaeger_span_ref_protobuf_destroy,
                            Jaegertracing__Protobuf__SpanRef)

static inline bool
jaeger_span_ref_to_protobuf(Jaegertracing__Protobuf__SpanRef* restrict dst,
                            const jaeger_span_ref* restrict src)
{
    assert(dst != NULL);
    assert(src != NULL);
    *dst = (Jaegertracing__Protobuf__SpanRef)
        JAEGERTRACING__PROTOBUF__SPAN_REF__INIT;
    dst->trace_id = (Jaegertracing__Protobuf__TraceID*) jaeger_malloc(
        sizeof(Jaegertracing__Protobuf__TraceID));
    if (dst->trace_id == NULL) {
        jaeger_log_error("Cannot allocate trace ID for span ref message");
        return false;
    }
    *dst->trace_id = (Jaegertracing__Protobuf__TraceID)
        JAEGERTRACING__PROTOBUF__TRACE_ID__INIT;
    jaeger_trace_id_to_protobuf(dst->trace_id, &src->context.trace_id);
#ifdef JAEGERTRACINGC_HAVE_PROTOBUF_OPTIONAL_FIELDS
    dst->has_type = true;
#endif /* JAEGERTRACINGC_HAVE_PROTOBUF_OPTIONAL_FIELDS */
    dst->type = src->type;
    dst->span_id = src->context.span_id;
    return true;
}

JAEGERTRACINGC_WRAP_COPY(jaeger_span_ref_to_protobuf,
                         Jaegertracing__Protobuf__SpanRef,
                         jaeger_span_ref)

struct jaeger_tracer;

typedef struct jaeger_span {
    struct jaeger_tracer* tracer;
    jaeger_span_context context;
    char* operation_name;
    jaeger_timestamp start_time_system;
    jaeger_duration start_time_steady;
    jaeger_duration duration;
    jaeger_vector tags;
    jaeger_vector logs;
    jaeger_vector refs;
    jaeger_mutex mutex;
} jaeger_span;

#define JAEGERTRACINGC_SPAN_INIT                                               \
    {                                                                          \
        .tracer = NULL, .context = JAEGERTRACINGC_SPAN_CONTEXT_INIT,           \
        .operation_name = NULL,                                                \
        .start_time_system = JAEGERTRACINGC_TIMESTAMP_INIT,                    \
        .start_time_steady = JAEGERTRACINGC_DURATION_INIT,                     \
        .duration = JAEGERTRACINGC_DURATION_INIT,                              \
        .tags = JAEGERTRACINGC_VECTOR_INIT,                                    \
        .logs = JAEGERTRACINGC_VECTOR_INIT,                                    \
        .refs = JAEGERTRACINGC_VECTOR_INIT, .mutex = JAEGERTRACINGC_MUTEX_INIT \
    }

static inline void jaeger_span_destroy(jaeger_span* span)
{
    if (span == NULL) {
        return;
    }

    jaeger_mutex_lock(&span->mutex);
    if (span->tracer != NULL) {
        span->tracer = NULL;
    }
    if (span->operation_name != NULL) {
        jaeger_free(span->operation_name);
        span->operation_name = NULL;
    }
    JAEGERTRACINGC_VECTOR_FOR_EACH(&span->tags, jaeger_tag_destroy, jaeger_tag);
    JAEGERTRACINGC_VECTOR_FOR_EACH(
        &span->logs, jaeger_log_record_destroy, jaeger_log_record);
    JAEGERTRACINGC_VECTOR_FOR_EACH(
        &span->refs, jaeger_span_ref_destroy, jaeger_span_ref);
    jaeger_span_context_destroy(&span->context);
    jaeger_vector_destroy(&span->tags);
    jaeger_vector_destroy(&span->logs);
    jaeger_vector_destroy(&span->refs);
    jaeger_mutex_unlock(&span->mutex);
    jaeger_mutex_destroy(&span->mutex);
}

static inline bool jaeger_span_init_vectors(jaeger_span* span)
{
    if (!jaeger_vector_init(&span->tags, sizeof(jaeger_tag))) {
        return false;
    }
    if (!jaeger_vector_init(&span->logs, sizeof(jaeger_log_record))) {
        return false;
    }
    if (!jaeger_vector_init(&span->refs, sizeof(jaeger_span_ref))) {
        return false;
    }
    return true;
}

/**
 * @internal
 * Initialize a new span. This function should not be called directly.
 * Instead,
 * use jaeger_tracer_start_span to start a new span.
 * @param span Span to initialize. May not be NULL.
 * @return True on success, false otherwise.
 * @see jaeger_tracer_start_span()
 */
static inline bool jaeger_span_init(jaeger_span* span)
{
    assert(span != NULL);
    *span = (jaeger_span) JAEGERTRACINGC_SPAN_INIT;
    if (!jaeger_span_context_init(&span->context)) {
        return false;
    }
    if (!jaeger_span_init_vectors(span)) {
        goto cleanup;
    }
    return true;

cleanup:
    jaeger_span_destroy(span);
    return false;
}

static inline bool jaeger_span_copy(jaeger_span* restrict dst,
                                    const jaeger_span* restrict src)
{
    assert(dst != NULL);
    assert(src != NULL);
    *dst = (jaeger_span) JAEGERTRACINGC_SPAN_INIT;
    if (!jaeger_span_init_vectors(dst)) {
        return false;
    }
    jaeger_lock(&dst->mutex, (jaeger_mutex*) &src->mutex);
    dst->start_time_system = src->start_time_system;
    dst->start_time_steady = src->start_time_steady;
    dst->duration = src->duration;
    dst->operation_name = jaeger_strdup(src->operation_name);
    if (!jaeger_span_context_copy(&dst->context, &src->context)) {
        goto cleanup;
    }
    if (dst->operation_name == NULL) {
        goto cleanup;
    }
    if (!jaeger_vector_copy(
            &dst->tags, &src->tags, &jaeger_tag_copy_wrapper, NULL)) {
        goto cleanup;
    }

    if (!jaeger_vector_copy(&dst->logs,
                            &src->logs,
                            &jaeger_log_record_copy_wrapper,
                            &jaeger_log_record_destroy_wrapper)) {
        goto cleanup;
    }

    if (!jaeger_vector_copy(
            &dst->refs, &src->refs, &jaeger_span_ref_copy, NULL)) {
        goto cleanup;
    }

    jaeger_mutex_unlock(&dst->mutex);
    jaeger_mutex_unlock((jaeger_mutex*) &src->mutex);
    return true;

cleanup:
    jaeger_mutex_unlock((jaeger_mutex*) &src->mutex);
    jaeger_mutex_unlock(&dst->mutex);
    jaeger_span_destroy(dst);
    return false;
}

/**
 * Get the sampling status of the span. Does not need to lock mutex because
 * span
 * context is immutable.
 * @param span The span instance.
 * @return True if sampled, false otherwise.
 */
static inline bool jaeger_span_is_sampled(const jaeger_span* span)
{
    assert(span != NULL);
    return (span->context.flags & jaeger_sampling_flag_sampled) != 0;
}

/**
 * Set operation name of span.
 * @param span The span instance.
 * @param operation_name The new operation name.
 * @return True on success, false otherwise.
 */
static inline bool jaeger_span_set_operation_name(jaeger_span* span,
                                                  const char* operation_name)
{
    assert(span != NULL);
    assert(operation_name != NULL);
    bool success = true;
    jaeger_mutex_lock(&span->mutex);
    if (!jaeger_span_is_sampled(span)) {
        goto cleanup;
    }

    char* operation_name_copy = jaeger_strdup(operation_name);
    if (operation_name_copy == NULL) {
        success = false;
        goto cleanup;
    }

    if (span->operation_name != NULL) {
        jaeger_free(span->operation_name);
    }
    span->operation_name = operation_name_copy;
    goto cleanup;

cleanup:
    jaeger_mutex_unlock(&span->mutex);
    return success;
}

/**
 * Add tag to span without locking.
 * @param span The span instance.
 * @param tag The tag to append.
 * @return True on success, false, otherwise.
 * @see jaeger_span_set_tag()
 */
static inline bool jaeger_span_set_tag_no_locking(jaeger_span* span,
                                                  const jaeger_tag* tag)
{
    jaeger_tag* tag_copy = (jaeger_tag*) jaeger_vector_append(&span->tags);
    if (tag_copy == NULL) {
        return false;
    }
    if (!jaeger_tag_copy(tag_copy, tag)) {
        span->tags.len--;
        return false;
    }
    return true;
}

/**
 * Set sampling flag on span.
 * @param span The span instance.
 * @param tag The tag value to consider for sampling.
 * @return True on success, false otherwise.
 */
static inline bool jaeger_span_set_sampling_priority(jaeger_span* span,
                                                     const jaeger_tag* tag)
{
    assert(span != NULL);
    assert(tag != NULL);
    if (tag->value_case != JAEGERTRACINGC_TAG_TYPE(LONG)) {
        return false;
    }
    jaeger_mutex_lock(&span->mutex);
    const bool success = (tag->long_value == 0);
    if (success) {
        span->context.flags = span->context.flags | jaeger_sampling_flag_debug |
                              jaeger_sampling_flag_sampled;
    }
    else {
        span->context.flags =
            span->context.flags & (~jaeger_sampling_flag_sampled);
    }
    jaeger_mutex_unlock(&span->mutex);
    return success;
}

/**
 * Add tag to span.
 * @param span The span instance.
 * @param tag The tag to append.
 * @return True on success, false otherwise.
 */
static inline bool jaeger_span_set_tag(jaeger_span* span, const jaeger_tag* tag)
{
    assert(span != NULL);
    assert(tag != NULL);
    if (strcmp(tag->key, JAEGERTRACINGC_SAMPLING_PRIORITY) == 0 &&
        !jaeger_span_set_sampling_priority(span, tag)) {
        return false;
    }
    bool success = true;
    jaeger_mutex_lock(&span->mutex);
    if (jaeger_span_is_sampled(span)) {
        success = jaeger_span_set_tag_no_locking(span, tag);
    }
    jaeger_mutex_unlock(&span->mutex);
    return success;
}

/**
 * Append to span logs without locking.
 * @param span The span instance.
 * @param log_record The log record to append.
 * @return True on success, false otherwise.
 * @see jaeger_span_log()
 */
static inline bool
jaeger_span_log_no_locking(jaeger_span* span,
                           const jaeger_log_record* log_record)
{
    jaeger_log_record* log_record_copy =
        (jaeger_log_record*) jaeger_vector_append(&span->logs);
    if (log_record_copy == NULL) {
        return false;
    }
    *log_record_copy = (jaeger_log_record) JAEGERTRACINGC_LOG_RECORD_INIT;
    if (!jaeger_log_record_copy(log_record_copy, log_record)) {
        goto cleanup;
    }
    return true;

cleanup:
    span->logs.len--;
    return false;
}

/**
 * Append to span logs.
 * @param span The span instance.
 * @param log_record The log record to append.
 * @return True on success, false otherwise.
 */
static inline bool jaeger_span_log(jaeger_span* span,
                                   const jaeger_log_record* log_record)
{
    assert(span != NULL);
    assert(log_record != NULL);
    bool success = true;
    jaeger_mutex_lock(&span->mutex);
    if (jaeger_span_is_sampled(span)) {
        success = jaeger_span_log_no_locking(span, log_record);
    }
    jaeger_mutex_unlock(&span->mutex);
    return success;
}

/** Options caller may set for span finish. */
typedef struct jaeger_span_finish_options {
    /**
     * The time the operation was completed. Must be initialized with
     * monotonic/steady clock.
     */
    jaeger_duration finish_time;
    /** Logs to append to the span logs. */
    const jaeger_log_record* logs;
    /** Number of logs to append to the span logs. */
    int num_logs;
} jaeger_span_finish_options;

/** Static initializer for jaeger_span_finish_with_options. */
#define JAEGERTRACINGC_SPAN_FINISH_OPTIONS_INIT                     \
    {                                                               \
        .finish_time = JAEGERTRACINGC_TIMESTAMP_INIT, .logs = NULL, \
        .num_logs = 0                                               \
    }

/* Forward declaration */
void jaeger_tracer_report_span(struct jaeger_tracer* tracer, jaeger_span* span);

/** Finish span with options.
 * @param span The span instance.
 * @param options Options to determine finish timestamp, etc. May be NULL.
 */
static inline void
jaeger_span_finish_with_options(jaeger_span* span,
                                const jaeger_span_finish_options* options)
{
    assert(span != NULL);
    if (options == NULL) {
        jaeger_span_finish_options default_finish_options =
            JAEGERTRACINGC_SPAN_FINISH_OPTIONS_INIT;
        jaeger_span_finish_with_options(span, &default_finish_options);
        return;
    }
    if (jaeger_span_is_sampled(span)) {
        jaeger_mutex_lock(&span->mutex);
        jaeger_duration finish_time = options->finish_time;
        if (finish_time.value.tv_sec == 0 && finish_time.value.tv_nsec == 0) {
            jaeger_duration_now(&finish_time);
        }
        jaeger_duration elapsed_time = JAEGERTRACINGC_DURATION_INIT;
        jaeger_duration_subtract(
            &finish_time, &span->start_time_steady, &elapsed_time);
        span->duration = elapsed_time;

        assert(options->num_logs == 0 || options->logs != NULL);
        for (int i = 0; i < options->num_logs; i++) {
            jaeger_span_log_no_locking(span, &options->logs[i]);
        }
        jaeger_mutex_unlock(&span->mutex);
    }

    /* Call jaeger_tracer_report_span even for non-sampled traces, in case
     * we
     * need to return the span to a pool.
     */
    jaeger_tracer_report_span(span->tracer, span);
}

static inline void
jaeger_protobuf_list_destroy(void** data, int num, void (*destroy)(void*))
{
    if (num == 0) {
        return;
    }
    assert(num > 0);
    assert(data != NULL);
    assert(destroy != NULL);
    for (int i = 0; i < (int) num; i++) {
        if (data[i] == NULL) {
            continue;
        }
        destroy(data[i]);
        jaeger_free(data[i]);
    }
    jaeger_free(data);
}

static inline void
jaeger_span_protobuf_destroy(Jaegertracing__Protobuf__Span* span)
{
    if (span == NULL) {
        return;
    }
    if (span->trace_id != NULL) {
        jaeger_free(span->trace_id);
        span->trace_id = NULL;
    }
    if (span->operation_name != NULL) {
        jaeger_free(span->operation_name);
        span->operation_name = NULL;
    }
    if (span->references != NULL) {
        jaeger_protobuf_list_destroy((void**) span->references,
                                     span->n_references,
                                     &jaeger_span_ref_protobuf_destroy_wrapper);
        span->references = NULL;
    }
    if (span->logs != NULL) {
        jaeger_protobuf_list_destroy(
            (void**) span->logs,
            span->n_logs,
            &jaeger_log_record_protobuf_destroy_wrapper);
        span->logs = NULL;
    }
    if (span->tags != NULL) {
        jaeger_protobuf_list_destroy(
            (void**) span->tags, span->n_tags, &jaeger_tag_destroy_wrapper);
        span->tags = NULL;
    }
}

static inline bool
jaeger_span_to_protobuf(Jaegertracing__Protobuf__Span* restrict dst,
                        const jaeger_span* restrict src)
{
    assert(dst != NULL);
    assert(src != NULL);
    *dst = (Jaegertracing__Protobuf__Span) JAEGERTRACING__PROTOBUF__SPAN__INIT;
    dst->trace_id = (Jaegertracing__Protobuf__TraceID*) jaeger_malloc(
        sizeof(Jaegertracing__Protobuf__TraceID));
    if (dst->trace_id == NULL) {
        jaeger_log_error("Cannot allocate trace ID for span message");
        return false;
    }
    *dst->trace_id = (Jaegertracing__Protobuf__TraceID)
        JAEGERTRACING__PROTOBUF__TRACE_ID__INIT;
#ifdef JAEGERTRACINGC_HAVE_PROTOBUF_OPTIONAL_FIELDS
    dst->has_span_id = true;
    dst->has_parent_span_id = true;
#endif /* JAEGERTRACINGC_HAVE_PROTOBUF_OPTIONAL_FIELDS */
    jaeger_mutex_lock((jaeger_mutex*) &src->mutex);
    jaeger_trace_id_to_protobuf(dst->trace_id, &src->context.trace_id);
    dst->span_id = src->context.span_id;
    dst->parent_span_id = src->context.parent_id;
    dst->operation_name = jaeger_strdup(src->operation_name);
    if (dst->operation_name == NULL) {
        goto cleanup;
    }

    if (!jaeger_vector_protobuf_copy((void***) &dst->tags,
                                     &dst->n_tags,
                                     &src->tags,
                                     sizeof(jaeger_tag),
                                     &jaeger_tag_copy_wrapper,
                                     &jaeger_tag_destroy_wrapper,
                                     NULL)) {
        goto cleanup;
    }
    if (!jaeger_vector_protobuf_copy(
            (void***) &dst->logs,
            &dst->n_logs,
            &src->logs,
            sizeof(Jaegertracing__Protobuf__Log),
            &jaeger_log_record_to_protobuf_wrapper,
            &jaeger_log_record_protobuf_destroy_wrapper,
            NULL)) {
        goto cleanup;
    }
    if (!jaeger_vector_protobuf_copy((void***) &dst->references,
                                     &dst->n_references,
                                     &src->refs,
                                     sizeof(Jaegertracing__Protobuf__SpanRef),
                                     &jaeger_span_ref_to_protobuf_wrapper,
                                     &jaeger_span_ref_protobuf_destroy_wrapper,
                                     NULL)) {
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
