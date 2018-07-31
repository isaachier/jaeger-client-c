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
#include "jaegertracingc/hashtable.h"
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
    (JAEGERTRACINGC_TRACE_ID_MAX_STR_LEN + 21)

#define JAEGERTRACINGC_SAMPLING_PRIORITY "sampling.priority"

enum {
    jaeger_sampling_flag_sampled = 1u,
    jaeger_sampling_flag_debug = (1u << 1u)
};

/**
 * Span context represents propagated span identity and state.
 */
typedef struct jaeger_span_context {
    /** Base class member. */
    opentracing_span_context base;

    /** Trace ID of the trace containing this span. */
    jaeger_trace_id trace_id;

    /**
     * Randomly generated unique ID. Must be unique within its trace, but not
     * necessarily globally unique.
     */
    uint64_t span_id;

    /** Sampling flags. */
    uint8_t flags;

    /** Key-value pairs that are propagated along with the span. */
    jaeger_hashtable baggage;

    /**
     * Can be set to correlation ID when the context is being extracted from a
     * carrier.
     * @see JAEGERTRACINGC_DEBUG_HEADER
     */
    char* debug_id;

    /**
     * Lock to protect mutable members.
     * @see baggage
     */
    jaeger_mutex mutex;
} jaeger_span_context;

static const char jaeger_span_context_type_descriptor[] = "jaeger_span_context";
static const int jaeger_span_context_type_descriptor_length =
    sizeof(jaeger_span_context_type_descriptor);

#define JAEGERTRACINGC_SPAN_CONTEXT_INIT                                    \
    {                                                                       \
        .base = {.base = {.destroy = &jaeger_span_context_destroy},         \
                 .foreach_baggage_item =                                    \
                     &jaeger_span_context_foreach_baggage_item,             \
                 .type_descriptor = jaeger_span_context_type_descriptor,    \
                 .type_descriptor_length =                                  \
                     jaeger_span_context_type_descriptor_length},           \
        .trace_id = JAEGERTRACINGC_TRACE_ID_INIT, .span_id = 0, .flags = 0, \
        .baggage = JAEGERTRACINGC_HASHTABLE_INIT, .debug_id = NULL,         \
        .mutex = JAEGERTRACINGC_MUTEX_INIT                                  \
    }

void jaeger_span_context_destroy(jaeger_destructible* d);

void jaeger_span_context_foreach_baggage_item(
    opentracing_span_context* span_context,
    opentracing_bool (*f)(void* arg, const char* key, const char* value),
    void* arg);

bool jaeger_span_context_init(jaeger_span_context* ctx);

bool jaeger_span_context_copy(jaeger_span_context* restrict dst,
                              const jaeger_span_context* restrict src);

/**
 * @internal
 * Returns whether or not the span context is valid.
 * @param ctx Span context instance. May not be NULL.
 * @return True if valid, false otherwise.
 */
bool jaeger_span_context_is_valid(const jaeger_span_context* ctx);

/**
 * @internal
 * Determines whether or not a span context is only used to return the
 * debug/correlation ID from extract() method. This happens in the situation
 * when "jaeger-debug-id" header is passed in the carrier to the extract()
 * method, but the request otherwise has no span context in it. Previously
 * this would have returned an error (opentracing_propagation_error_code) from
 * the extract method, but now it returns a dummy context with only debug_id
 * filled in.
 * @param ctx Span context instance. May not be NULL.
 * @return True if just debug container, false otherwise.
 */
bool jaeger_span_context_is_debug_id_container_only(
    const jaeger_span_context* ctx);

typedef opentracing_span_reference_type jaeger_span_ref_type;

typedef struct jaeger_span_ref {
    jaeger_span_context context;
    jaeger_span_ref_type type;
} jaeger_span_ref;

bool jaeger_span_ref_init(jaeger_span_ref* span_ref);

void jaeger_span_ref_destroy(jaeger_span_ref* span_ref);

bool jaeger_span_ref_copy(void* arg,
                          void* restrict dst,
                          const void* restrict src);

void jaeger_span_ref_protobuf_destroy(Jaeger__Model__SpanRef* span_ref);

JAEGERTRACINGC_WRAP_DESTROY(jaeger_span_ref_protobuf_destroy,
                            Jaeger__Model__SpanRef)

bool jaeger_span_ref_to_protobuf(Jaeger__Model__SpanRef* restrict dst,
                                 const jaeger_span_ref* restrict src);

JAEGERTRACINGC_WRAP_COPY(jaeger_span_ref_to_protobuf,
                         Jaeger__Model__SpanRef,
                         jaeger_span_ref)

/* Forward declaration */
struct jaeger_tracer;

typedef struct jaeger_span {
    /** Base class member. */
    opentracing_span base;
    /** Tracer that creates this span. */
    struct jaeger_tracer* tracer;
    /** Span context. */
    jaeger_span_context context;
    /** Operation represented by this span. */
    char* operation_name;
    /** Start time using system clock. */
    jaeger_timestamp start_time_system;
    /** Start time using monotonic/steady clock. */
    jaeger_duration start_time_steady;
    /** Overall duration of span (set on span finish). */
    jaeger_duration duration;
    /** Span tags. */
    jaeger_vector tags;
    /** Span log records. */
    jaeger_vector logs;
    /** Span context references (i.e. CHILD_OF and/or FOLLOWS_FROM). */
    jaeger_vector refs;
    /** Mutex to guard mutable members. */
    jaeger_mutex mutex;
} jaeger_span;

/* Forward declaration. */
void jaeger_tracer_report_span(struct jaeger_tracer* tracer, jaeger_span* span);

/** Static initializer for opentracing_finish_span_options. */
#define JAEGERTRACINGC_FINISH_SPAN_OPTIONS_INIT                            \
    {                                                                      \
        .finish_time = JAEGERTRACINGC_TIMESTAMP_INIT, .log_records = NULL, \
        .num_log_records = 0                                               \
    }

void jaeger_span_destroy(jaeger_destructible* d);

bool jaeger_span_init_vectors(jaeger_span* span);

/**
 * Get the sampling status of the span without using mutex. Use at your own
 * risk.
 * @param span The span instance.
 * @return True if sampled, false otherwise.
 */
bool jaeger_span_is_sampled_no_locking(const jaeger_span* span);

/**
 * Append to span logs without locking.
 * @param span The span instance.
 * @param log_record The log record to append.
 * @see jaeger_span_log()
 */
void jaeger_span_log_no_locking(jaeger_span* span,
                                const opentracing_log_record* log_record);

/**
 * Finish span with options.
 * @param span The span instance.
 * @param options Options to determine finish timestamp, etc. May be NULL.
 */
void jaeger_span_finish_with_options(
    opentracing_span* s, const opentracing_finish_span_options* options);

/**
 * Finish span.
 * @param span The span instance.
 */
void jaeger_span_finish(opentracing_span* span);

/**
 * Get the sampling status of the span. Uses lock to protect flags.
 * @param span The span instance.
 * @return True if sampled, false otherwise.
 */
bool jaeger_span_is_sampled(const jaeger_span* span);

/**
 * Set operation name of span.
 * @param span The span instance.
 * @param operation_name The new operation name.
 * @return True on success, false otherwise.
 */
void jaeger_span_set_operation_name(opentracing_span* s,
                                    const char* operation_name);

/**
 * Set baggage item.
 * @param span Span instance.
 * @param key Baggage item key.
 * @param value Baggage item value.
 */
void jaeger_span_set_baggage_item(opentracing_span* span,
                                  const char* key,
                                  const char* value);

/**
 * Get baggage item from span for given key.
 * @param span Span instance.
 * @param key Baggage item key.
 * @return If key in baggage, returns value corresponding to key, returns NULL
otherwise.
 */
const char* jaeger_span_baggage_item(const opentracing_span* span,
                                     const char* key);

/**
 * Add tag to span without locking.
 * @param span The span instance.
 * @param tag The tag to append.
 * @return True on success, false, otherwise.
 * @see jaeger_span_set_tag()
 */
void jaeger_span_set_tag_no_locking(jaeger_span* span,
                                    const char* key,
                                    const opentracing_value* value);

/**
 * Set sampling flag on span.
 * @internal
 * @param span The span instance.
 * @param tag The tag value to consider for sampling.
 * @return True on success, false otherwise.
 */
bool jaeger_span_set_sampling_priority(jaeger_span* span,
                                       const opentracing_value* value);

/**
 * Add tag to span.
 * @param span The span instance.
 * @param tag The tag to append.
 * @return True on success, false otherwise.
 */
void jaeger_span_set_tag(opentracing_span* span,
                         const char* key,
                         const opentracing_value* value);

/**
 * Append to span logs.
 * @param span The span instance.
 * @param fields The log fields for the record to append.
 * @param num_fields The number of log fields. May only be zero if fields is
 *                   NULL.
 */
void jaeger_span_log(opentracing_span* span,
                     const opentracing_log_field* fields,
                     int num_fields);

/* Static initializer for span. */
#define JAEGERTRACINGC_SPAN_INIT                                               \
    {                                                                          \
        .base = {.base = {.destroy = &jaeger_span_destroy},                    \
                 .finish = &jaeger_span_finish,                                \
                 .finish_with_options = &jaeger_span_finish_with_options,      \
                 .set_operation_name = &jaeger_span_set_operation_name,        \
                 .set_tag = &jaeger_span_set_tag,                              \
                 .log_fields = &jaeger_span_log,                               \
                 .set_baggage_item = &jaeger_span_set_baggage_item,            \
                 .baggage_item = &jaeger_span_baggage_item},                   \
        .tracer = NULL, .context = JAEGERTRACINGC_SPAN_CONTEXT_INIT,           \
        .operation_name = NULL,                                                \
        .start_time_system = JAEGERTRACINGC_TIMESTAMP_INIT,                    \
        .start_time_steady = JAEGERTRACINGC_DURATION_INIT,                     \
        .duration = JAEGERTRACINGC_DURATION_INIT,                              \
        .tags = JAEGERTRACINGC_VECTOR_INIT,                                    \
        .logs = JAEGERTRACINGC_VECTOR_INIT,                                    \
        .refs = JAEGERTRACINGC_VECTOR_INIT, .mutex = JAEGERTRACINGC_MUTEX_INIT \
    }

/**
 * @internal
 * Initialize a new span. This function should not be called directly.
 * Instead, use jaeger_tracer_start_span to start a new span.
 * @param span Span to initialize. May not be NULL.
 * @return True on success, false otherwise.
 * @see jaeger_tracer_start_span()
 */
bool jaeger_span_init(jaeger_span* span);

bool jaeger_span_copy(jaeger_span* restrict dst,
                      const jaeger_span* restrict src);

void jaeger_protobuf_list_destroy(void** data, int num, void (*destroy)(void*));

void jaeger_span_protobuf_destroy(Jaeger__Model__Span* span);

bool jaeger_span_to_protobuf(Jaeger__Model__Span* restrict dst,
                             const jaeger_span* restrict src);

int jaeger_span_context_format(const jaeger_span_context* ctx,
                               char* buffer,
                               int buffer_len);

bool jaeger_span_context_scan(jaeger_span_context* ctx, const char* str);

#ifdef __cplusplus
} /* extern C */
#endif /* __cplusplus */

#endif /* JAEGERTRACINGC_SPAN_H */
