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

#include "jaegertracingc/common.h"
#include "jaegertracingc/duration.h"
#include "jaegertracingc/tag.h"
#include "jaegertracingc/threading.h"
#include "jaegertracingc/trace_id.h"
#include "jaegertracingc/vector.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct jaeger_log_record {
    time_t timestamp;
    jaeger_tag_list tags;
} jaeger_log_record;

static inline bool jaeger_log_record_init(jaeger_log_record* log_record,
                                          jaeger_logger* logger)
{
    assert(log_record != NULL);
    time(&log_record->timestamp);
    return jaeger_tag_list_init(&log_record->tags, logger);
}

static inline void jaeger_log_record_destroy(jaeger_log_record* log_record)
{
    if (log_record != NULL) {
        jaeger_tag_list_destroy(&log_record->tags);
    }
}

typedef struct jaeger_span_context {
    jaeger_trace_id trace_id;
    uint64_t span_id;
    uint64_t parent_id;
    uint8_t flags;
} jaeger_span_context;

enum { jaeger_span_ref_child_of = 0, jaeger_span_ref_follows_from = 1 };

typedef struct jaeger_span_ref {
    jaeger_span_context context;
    int type;
} jaeger_span_ref;

typedef struct jaeger_span {
    jaeger_span_context context;
    char* operation_name;
    time_t start_time_system;
    jaeger_duration start_time_steady;
    jaeger_duration duration;
    jaeger_tag_list tags;
    jaeger_vector logs;
    jaeger_vector refs;
    jaeger_mutex mutex;
} jaeger_span;

static inline bool jaeger_span_init(jaeger_span* span, jaeger_logger* logger)
{
    assert(span != NULL);
    memset(span, 0, sizeof(*span));
    if (!jaeger_tag_list_init(&span->tags, logger)) {
        return false;
    }
    if (!jaeger_vector_init(
            &span->logs, sizeof(jaeger_log_record), NULL, logger)) {
        goto cleanup_tags;
    }
    if (!jaeger_vector_init(
            &span->refs, sizeof(jaeger_span_ref), NULL, logger)) {
        goto cleanup_logs;
    }
    span->mutex = (jaeger_mutex) JAEGERTRACINGC_MUTEX_INIT;
    return true;

cleanup_logs:
    JAEGERTRACINGC_VECTOR_FOR_EACH(&span->refs, jaeger_log_record_destroy);
cleanup_tags:
    jaeger_tag_list_destroy(&span->tags);
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
