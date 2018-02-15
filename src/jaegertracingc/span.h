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
#include "jaegertracingc/trace_id.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct jaeger_span {
    jaeger_trace_id trace_id;
    uint64_t span_id;
    uint64_t parent_id;
    uint8_t flags;
} jaeger_span;

static inline int
jaeger_span_format(const jaeger_span* span, char* buffer, int buffer_len)
{
    assert(span != NULL);
    assert(buffer != NULL);
    assert(buffer_len >= 0);
    const int trace_id_len =
        jaeger_trace_id_format(&span->trace_id, buffer, buffer_len);
    if (trace_id_len > buffer_len) {
        return trace_id_len + snprintf(NULL,
                                       0,
                                       ":%" PRIx64 ":%" PRIx64 ":%" PRIx8,
                                       span->span_id,
                                       span->parent_id,
                                       span->flags);
    }
    return snprintf(&buffer[trace_id_len],
                    buffer_len - trace_id_len,
                    ":%" PRIx64 ":%" PRIx64 ":%" PRIx8,
                    span->span_id,
                    span->parent_id,
                    span->flags);
}

static inline bool jaeger_span_scan(jaeger_span* span, const char* str)
{
    assert(span != NULL);
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

    *span = (jaeger_span){.trace_id = trace_id,
                          .span_id = span_id,
                          .parent_id = parent_id,
                          .flags = flags};
    return true;
}

#ifdef __cplusplus
} /* extern C */
#endif /* __cplusplus */

#endif /* JAEGERTRACINGC_SPAN_H */
