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

#include "jaegertracingc/span.h"

void jaeger_span_context_destroy(jaeger_destructible* d)
{
    if (d == NULL) {
        return;
    }
    jaeger_span_context* ctx = (jaeger_span_context*) d;
    jaeger_hashtable_destroy(&ctx->baggage);
    if (ctx->debug_id != NULL) {
        jaeger_free(ctx->debug_id);
        ctx->debug_id = NULL;
    }
    jaeger_mutex_destroy(&ctx->mutex);
}

void jaeger_span_context_foreach_baggage_item(
    opentracing_span_context* span_context,
    opentracing_bool (*f)(void* arg, const char* key, const char* value),
    void* arg)
{
    jaeger_span_context* ctx = (jaeger_span_context*) span_context;
    jaeger_mutex_lock(&ctx->mutex);

    jaeger_hashtable* baggage = &((jaeger_span_context*) span_context)->baggage;
    for (size_t i = 0, len = 1u << baggage->order; i < len; i++) {
        for (const jaeger_list_node* node = baggage->buckets[i].head;
             node != NULL;
             node = node->next) {
            const jaeger_key_value* kv = &((jaeger_key_value_node*) node)->data;
            if (!f(arg, kv->key, kv->value)) {
                break;
            }
        }
    }

    jaeger_mutex_unlock(&ctx->mutex);
}

bool jaeger_span_context_init(jaeger_span_context* ctx)
{
    assert(ctx != NULL);
    *ctx = (jaeger_span_context) JAEGERTRACINGC_SPAN_CONTEXT_INIT;
    return jaeger_hashtable_init(&ctx->baggage);
}

bool jaeger_span_context_copy(jaeger_span_context* restrict dst,
                              const jaeger_span_context* restrict src)
{
    assert(dst != NULL);
    assert(src != NULL);
    *dst = (jaeger_span_context) JAEGERTRACINGC_SPAN_CONTEXT_INIT;
    jaeger_lock((jaeger_mutex*) &src->mutex, &dst->mutex);
    if (!jaeger_hashtable_copy(&dst->baggage, &src->baggage)) {
        jaeger_mutex_unlock((jaeger_mutex*) &src->mutex);
        jaeger_mutex_unlock(&dst->mutex);
        jaeger_span_context_destroy((jaeger_destructible*) dst);
        return false;
    }
    dst->trace_id = src->trace_id;
    dst->span_id = src->span_id;
    dst->flags = src->flags;
    jaeger_mutex_unlock((jaeger_mutex*) &src->mutex);
    jaeger_mutex_unlock(&dst->mutex);
    return true;
}

bool jaeger_span_context_is_valid(const jaeger_span_context* ctx)
{
    assert(ctx != NULL);
    return ctx->trace_id.high != 0 || ctx->trace_id.low != 0;
}

bool jaeger_span_context_is_debug_id_container_only(
    const jaeger_span_context* ctx)
{
    assert(ctx != NULL);
    return !jaeger_span_context_is_valid(ctx) && ctx->debug_id != NULL;
}

bool jaeger_span_ref_init(jaeger_span_ref* span_ref)
{
    assert(span_ref != NULL);
    *span_ref = (jaeger_span_ref){.context = JAEGERTRACINGC_SPAN_CONTEXT_INIT,
                                  .type = (jaeger_span_ref_type) -1};
    return jaeger_span_context_init(&span_ref->context);
}

void jaeger_span_ref_destroy(jaeger_span_ref* span_ref)
{
    if (span_ref == NULL) {
        return;
    }

    jaeger_span_context_destroy((jaeger_destructible*) &span_ref->context);
}

bool jaeger_span_ref_copy(void* arg,
                          void* restrict dst,
                          const void* restrict src)
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

void jaeger_span_ref_protobuf_destroy(Jaeger__Model__SpanRef* span_ref)
{
    if (span_ref == NULL) {
        return;
    }

    jaeger_free(span_ref->trace_id.data);
    span_ref->trace_id = (ProtobufCBinaryData){.data = NULL, .len = 0};
    jaeger_free(span_ref->span_id.data);
    span_ref->span_id = (ProtobufCBinaryData){.data = NULL, .len = 0};
}

bool jaeger_span_ref_to_protobuf(Jaeger__Model__SpanRef* restrict dst,
                                 const jaeger_span_ref* restrict src)
{
    assert(dst != NULL);
    assert(src != NULL);
    *dst = (Jaeger__Model__SpanRef) JAEGER__MODEL__SPAN_REF__INIT;

    dst->trace_id =
        (ProtobufCBinaryData){.len = sizeof(uint64_t) * 2,
                              .data = jaeger_malloc(sizeof(uint64_t) * 2)};
    if (dst->trace_id.data == NULL) {
        jaeger_log_error("Cannot allocate trace ID for span ref message");
        return false;
    }

    dst->span_id = (ProtobufCBinaryData){
        .len = sizeof(src->context.span_id),
        .data = jaeger_malloc(sizeof(src->context.span_id))};
    if (dst->span_id.data == NULL) {
        jaeger_log_error("Cannot allocate span ID for span ref message");
        jaeger_free(dst->trace_id.data);
        dst->trace_id = (ProtobufCBinaryData){.data = NULL, .len = 0};
        dst->span_id = (ProtobufCBinaryData){.data = NULL, .len = 0};
        return false;
    }

    jaeger_mutex_lock((jaeger_mutex*) &src->context.mutex);
    jaeger_trace_id_to_protobuf(&dst->trace_id, &src->context.trace_id);
    memcpy(
        dst->span_id.data, &src->context.span_id, sizeof(src->context.span_id));
    dst->span_id.len = sizeof(src->context.span_id);
    jaeger_mutex_unlock((jaeger_mutex*) &src->context.mutex);
    dst->ref_type = (Jaeger__Model__SpanRefType) src->type;
    return true;
}

void jaeger_span_destroy(jaeger_destructible* d)
{
    if (d == NULL) {
        return;
    }

    jaeger_span* span = (jaeger_span*) d;
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
    jaeger_span_context_destroy((jaeger_destructible*) &span->context);
    jaeger_vector_destroy(&span->tags);
    jaeger_vector_destroy(&span->logs);
    jaeger_vector_destroy(&span->refs);
    jaeger_mutex_destroy(&span->mutex);
}

bool jaeger_span_init_vectors(jaeger_span* span)
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

bool jaeger_span_is_sampled_no_locking(const jaeger_span* span)
{
    return (span->context.flags & ((uint8_t) jaeger_sampling_flag_sampled)) !=
           0;
}

void jaeger_span_log_no_locking(jaeger_span* span,
                                const opentracing_log_record* log_record)
{
    jaeger_log_record* log_record_copy =
        (jaeger_log_record*) jaeger_vector_append(&span->logs);
    if (log_record_copy == NULL) {
        return;
    }
    *log_record_copy = (jaeger_log_record) JAEGERTRACINGC_LOG_RECORD_INIT;
    if (!jaeger_log_record_from_opentracing(log_record_copy, log_record)) {
        goto cleanup;
    }
    return;

cleanup:
    span->logs.len--;
}

void jaeger_span_finish_with_options(
    opentracing_span* s, const opentracing_finish_span_options* options)
{
    assert(s != NULL);
    jaeger_span* span = (jaeger_span*) s;
    if (options == NULL) {
        opentracing_finish_span_options default_finish_options =
            JAEGERTRACINGC_FINISH_SPAN_OPTIONS_INIT;
        jaeger_span_finish_with_options(s, &default_finish_options);
        return;
    }
    jaeger_mutex_lock(&span->mutex);
    if (jaeger_span_is_sampled_no_locking(span)) {
        jaeger_duration finish_time = options->finish_time;
        if (finish_time.value.tv_sec == 0 && finish_time.value.tv_nsec == 0) {
            jaeger_duration_now(&finish_time);
        }
        const bool result = jaeger_time_subtract(finish_time.value,
                                                 span->start_time_steady.value,
                                                 &span->duration.value);
        (void) result;
        assert(result);

        assert(options->num_log_records == 0 || options->log_records != NULL);
        for (int i = 0; i < options->num_log_records; i++) {
            jaeger_span_log_no_locking(span, &options->log_records[i]);
        }
    }
    jaeger_mutex_unlock(&span->mutex);

    /* Call jaeger_tracer_report_span even for non-sampled traces, in case
     * we need to return the span to a pool.
     */
    jaeger_tracer_report_span(span->tracer, span);
}

void jaeger_span_finish(opentracing_span* span)
{
    jaeger_span_finish_with_options(span, NULL);
}

bool jaeger_span_is_sampled(const jaeger_span* span)
{
    assert(span != NULL);
    jaeger_lock((jaeger_mutex*) &span->mutex,
                (jaeger_mutex*) &span->context.mutex);
    const bool result = jaeger_span_is_sampled_no_locking(span);
    jaeger_mutex_unlock((jaeger_mutex*) &span->mutex);
    jaeger_mutex_unlock((jaeger_mutex*) &span->context.mutex);
    return result;
}

void jaeger_span_set_operation_name(opentracing_span* s,
                                    const char* operation_name)
{
    assert(s != NULL);
    assert(operation_name != NULL);
    jaeger_span* span = (jaeger_span*) s;
    jaeger_mutex_lock(&span->mutex);
    if (!jaeger_span_is_sampled_no_locking(span)) {
        goto cleanup;
    }

    char* operation_name_copy = jaeger_strdup(operation_name);
    if (operation_name_copy == NULL) {
        goto cleanup;
    }

    if (span->operation_name != NULL) {
        jaeger_free(span->operation_name);
    }
    span->operation_name = operation_name_copy;
    goto cleanup;

cleanup:
    jaeger_mutex_unlock(&span->mutex);
}

void jaeger_span_set_baggage_item(opentracing_span* span,
                                  const char* key,
                                  const char* value)
{
    jaeger_span* s = (jaeger_span*) span;
    jaeger_lock(&s->mutex, &s->context.mutex);
    /* TODO: Use baggage setter for validation once implemented. */
    jaeger_hashtable_put(&s->context.baggage, key, value);
    jaeger_mutex_unlock(&s->mutex);
    jaeger_mutex_unlock(&s->context.mutex);
}

const char* jaeger_span_baggage_item(const opentracing_span* span,
                                     const char* key)
{
    const jaeger_span* s = (const jaeger_span*) span;
    jaeger_lock((jaeger_mutex*) &s->mutex, (jaeger_mutex*) &s->context.mutex);
    const jaeger_key_value* kv =
        jaeger_hashtable_find((jaeger_hashtable*) &s->context.baggage, key);
    jaeger_mutex_unlock((jaeger_mutex*) &s->mutex);
    jaeger_mutex_unlock((jaeger_mutex*) &s->context.mutex);
    if (kv == NULL) {
        return NULL;
    }
    return kv->value;
}

void jaeger_span_set_tag_no_locking(jaeger_span* span,
                                    const char* key,
                                    const opentracing_value* value)
{
    jaeger_tag* tag_copy = (jaeger_tag*) jaeger_vector_append(&span->tags);
    if (tag_copy == NULL) {
        return;
    }
    if (!jaeger_tag_from_key_value(tag_copy, key, value)) {
        span->tags.len--;
    }
}

bool jaeger_span_set_sampling_priority(jaeger_span* span,
                                       const opentracing_value* value)
{
    assert(span != NULL);
    assert(value != NULL);
    switch (value->type) {
    case opentracing_value_int64:
        break;
    case opentracing_value_uint64:
        break;
    default:
        return false;
    }

    bool success = false;
    jaeger_lock(&span->mutex, &span->context.mutex);
    if ((value->type == opentracing_value_int64 && value->value.int64_value) ||
        (value->type == opentracing_value_uint64 &&
         value->value.uint64_value)) {
        span->context.flags =
            (uint8_t)(span->context.flags |
                      ((uint8_t) jaeger_sampling_flag_debug)) |
            ((uint8_t) jaeger_sampling_flag_sampled);
        success = true;
    }
    else {
        span->context.flags =
            (uint8_t)(span->context.flags &
                      ((uint8_t) ~(uint8_t) jaeger_sampling_flag_sampled));
    }
    jaeger_mutex_unlock(&span->mutex);
    jaeger_mutex_unlock(&span->context.mutex);
    return success;
}

void jaeger_span_set_tag(opentracing_span* span,
                         const char* key,
                         const opentracing_value* value)
{
    assert(span != NULL);
    assert(key != NULL);
    assert(value != NULL);
    jaeger_span* s = (jaeger_span*) span;
    if (strcmp(key, JAEGERTRACINGC_SAMPLING_PRIORITY) == 0 &&
        !jaeger_span_set_sampling_priority(s, value)) {
        return;
    }
    jaeger_mutex_lock(&s->mutex);
    if (jaeger_span_is_sampled_no_locking(s)) {
        jaeger_span_set_tag_no_locking(s, key, value);
    }
    jaeger_mutex_unlock(&s->mutex);
}

void jaeger_span_log(opentracing_span* span,
                     const opentracing_log_field* fields,
                     int num_fields)
{
    assert(span != NULL);
    assert(fields != NULL || num_fields == 0);
    jaeger_timestamp timestamp;
    jaeger_timestamp_now(&timestamp);
    const opentracing_log_record log_record = {
        .fields = (opentracing_log_field*) fields,
        .num_fields = num_fields,
        .timestamp = timestamp};
    jaeger_span* s = (jaeger_span*) span;
    jaeger_mutex_lock(&s->mutex);
    if (jaeger_span_is_sampled_no_locking(s)) {
        jaeger_span_log_no_locking(s, &log_record);
    }
    jaeger_mutex_unlock(&s->mutex);
}

bool jaeger_span_init(jaeger_span* span)
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
    jaeger_span_destroy((jaeger_destructible*) span);
    return false;
}

bool jaeger_span_copy(jaeger_span* restrict dst,
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
    jaeger_span_destroy((jaeger_destructible*) dst);
    return false;
}

void jaeger_protobuf_list_destroy(void** data, int num, void (*destroy)(void*))
{
    if (num == 0) {
        return;
    }
    assert(num > 0);
    assert(data != NULL);
    assert(destroy != NULL);
    for (int i = 0; i < num; i++) {
        if (data[i] == NULL) {
            continue;
        }
        destroy(data[i]);
        jaeger_free(data[i]);
    }
    jaeger_free(data);
}

void jaeger_span_protobuf_destroy(Jaeger__Model__Span* span)
{
    if (span == NULL) {
        return;
    }
    jaeger_free(span->trace_id.data);
    span->trace_id = (ProtobufCBinaryData){.data = NULL, .len = 0};
    jaeger_free(span->span_id.data);
    span->span_id = (ProtobufCBinaryData){.data = NULL, .len = 0};
    jaeger_free(span->operation_name);
    span->operation_name = NULL;
    jaeger_protobuf_list_destroy((void**) span->references,
                                 span->n_references,
                                 &jaeger_span_ref_protobuf_destroy_wrapper);
    span->references = NULL;
    jaeger_protobuf_list_destroy((void**) span->logs,
                                 span->n_logs,
                                 &jaeger_log_record_protobuf_destroy_wrapper);
    span->logs = NULL;
    jaeger_protobuf_list_destroy(
        (void**) span->tags, span->n_tags, &jaeger_tag_destroy_wrapper);
    span->tags = NULL;
}

bool jaeger_span_to_protobuf(Jaeger__Model__Span* restrict dst,
                             const jaeger_span* restrict src)
{
    assert(dst != NULL);
    assert(src != NULL);
    *dst = (Jaeger__Model__Span) JAEGER__MODEL__SPAN__INIT;
    dst->trace_id = (ProtobufCBinaryData){
        .data = jaeger_malloc(sizeof(src->context.trace_id)),
        .len = sizeof(src->context.trace_id)};
    if (dst->trace_id.data == NULL) {
        goto cleanup;
    }
    dst->span_id = (ProtobufCBinaryData){
        .data = jaeger_malloc(sizeof(src->context.span_id)),
        .len = sizeof(src->context.span_id)};
    if (dst->span_id.data == NULL) {
        goto cleanup;
    }
    jaeger_lock((jaeger_mutex*) &src->mutex,
                (jaeger_mutex*) &src->context.mutex);
    jaeger_trace_id_to_protobuf(&dst->trace_id, &src->context.trace_id);
    memcpy(
        dst->span_id.data, &src->context.span_id, sizeof(src->context.span_id));

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
            sizeof(Jaeger__Model__Log),
            &jaeger_log_record_to_protobuf_wrapper,
            &jaeger_log_record_protobuf_destroy_wrapper,
            NULL)) {
        goto cleanup;
    }

    if (!jaeger_vector_protobuf_copy((void***) &dst->references,
                                     &dst->n_references,
                                     &src->refs,
                                     sizeof(Jaeger__Model__SpanRef),
                                     &jaeger_span_ref_to_protobuf_wrapper,
                                     &jaeger_span_ref_protobuf_destroy_wrapper,
                                     NULL)) {
        goto cleanup;
    }
    jaeger_mutex_unlock((jaeger_mutex*) &src->mutex);
    jaeger_mutex_unlock((jaeger_mutex*) &src->context.mutex);
    return true;

cleanup:
    jaeger_mutex_unlock((jaeger_mutex*) &src->mutex);
    jaeger_mutex_unlock((jaeger_mutex*) &src->context.mutex);
    jaeger_span_protobuf_destroy(dst);
    *dst = (Jaeger__Model__Span) JAEGER__MODEL__SPAN__INIT;
    return false;
}

int jaeger_span_context_format(const jaeger_span_context* ctx,
                               char* buffer,
                               int buffer_len)
{
    assert(ctx != NULL);
    assert(buffer != NULL);
    assert(buffer_len >= 0);
    const int trace_id_len =
        jaeger_trace_id_format(&ctx->trace_id, buffer, buffer_len);
    jaeger_mutex_lock((jaeger_mutex*) &ctx->mutex);
    const uint8_t flags = ctx->flags;
    jaeger_mutex_unlock((jaeger_mutex*) &ctx->mutex);
    if (trace_id_len > buffer_len) {
        return trace_id_len +
               snprintf(NULL, 0, ":%" PRIx64 ":%" PRIx8, ctx->span_id, flags);
    }
    return snprintf(&buffer[trace_id_len],
                    buffer_len - trace_id_len,
                    ":%" PRIx64 ":%" PRIx8,
                    ctx->span_id,
                    flags);
}

bool jaeger_span_context_scan(jaeger_span_context* ctx, const char* str)
{
    assert(ctx != NULL);
    assert(str != NULL);
    bool result = true;
    jaeger_trace_id trace_id = JAEGERTRACINGC_TRACE_ID_INIT;
    char* buffer = jaeger_strdup(str);
    if (buffer == NULL) {
        return false;
    }
    uint64_t span_id = 0;
    uint8_t flags = 0;
    char* token = buffer;
    char* token_context = NULL;
    char* token_end = NULL;
    for (int i = 0; i < 4; i++) {
        token = strtok_r(i == 0 ? token : NULL, ":", &token_context);
        if (token == NULL && i != 3) {
            result = false;
            goto cleanup;
        }
        token_end = NULL;
        switch (i) {
        case 0:
            if (!jaeger_trace_id_scan(&trace_id, token)) {
                result = false;
                goto cleanup;
            }
            break;
        case 1:
            span_id = strtoull(token, &token_end, 16);
            break;
        case 2:
            flags = strtoull(token, &token_end, 16);
            break;
        default:
            assert(i == 3);
            break;
        }
        if (token_end != NULL && *token_end != '\0') {
            result = false;
            goto cleanup;
        }
    }

    jaeger_mutex_lock(&ctx->mutex);
    ctx->trace_id = trace_id;
    ctx->span_id = span_id;
    ctx->flags = flags;
    jaeger_mutex_unlock(&ctx->mutex);

cleanup:
    jaeger_free(buffer);
    return result;
}
