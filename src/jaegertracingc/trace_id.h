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

#ifndef JAEGERTRACINGC_TRACE_ID_H
#define JAEGERTRACINGC_TRACE_ID_H

#include "jaegertracingc/common.h"
#include "jaegertracingc/protoc-gen/jaeger.pb-c.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/** 128 bit unique ID for a given trace. */
typedef struct jaeger_trace_id {
    /** Upper 64 bits of the trace ID. */
    uint64_t high;
    /** Lower 64 bits of the trace ID. */
    uint64_t low;
} jaeger_trace_id;

/** Static initializer for trace ID struct. */
#define JAEGERTRACINGC_TRACE_ID_INIT \
    {                                \
        .high = 0, .low = 0          \
    }

/** Max string length to represent trace ID (not including null byte). */
#define JAEGERTRACINGC_TRACE_ID_MAX_STR_LEN 32
/** Max string length to represent uint64_t as hex (not including null byte). */
#define JAEGERTRACINGC_UINT64_MAX_STR_LEN 16
/** Base number for hexadecimal (16). */
#define JAEGERTRACINGC_HEX_BASE 16

/**
 * Convert a trace ID to its Protobuf-C representation.
 * @param dst Protobuf-C output argument.
 * @param src jaeger_trace_id source argument.
 */
static inline void
jaeger_trace_id_to_protobuf(Jaegertracing__Protobuf__TraceID* dst,
                            const jaeger_trace_id* src)
{
    assert(dst != NULL);
    assert(src != NULL);
#ifdef JAEGERTRACINGC_HAVE_PROTOBUF_OPTIONAL_FIELDS
    dst->has_high = true;
    dst->has_low = true;
#endif /* JAEGERTRACINGC_HAVE_PROTOBUF_OPTIONAL_FIELDS */
    dst->high = src->high;
    dst->low = src->low;
}

/**
 * Format a trace ID into a string representation. The string representation can
 * be used to represent a trace ID in network protocols where ASCII characters
 * are required/preferred.
 * @param trace_id The trace ID to format.
 * @param buffer The output character buffer.
 * @param buffer_len The length of the output character buffer.
 * @return The number of characters needed to represent the entire trace ID,
 *         similar to the behavior of snprintf.
 */
static inline int jaeger_trace_id_format(const jaeger_trace_id* trace_id,
                                         char* buffer,
                                         int buffer_len)
{
    assert(trace_id != NULL);
    assert(buffer != NULL);
    assert(buffer_len >= 0);
    if (trace_id->high == 0) {
        return snprintf(buffer, buffer_len, "%" PRIx64, trace_id->low);
    }
    return snprintf(buffer,
                    buffer_len,
                    "%" PRIx64 "%016" PRIx64,
                    trace_id->high,
                    trace_id->low);
}

/**
 * Scan a string representation of a trace ID into a trace ID instance.
 * @param trace_id The output trace ID.
 * @param str The input string representation.
 * @return True on success, false otherwise.
 * @see jaeger_trace_id_format()
 */
static inline bool jaeger_trace_id_scan(jaeger_trace_id* trace_id,
                                        const char* str)
{
    assert(trace_id != NULL);
    assert(str != NULL);
    const int len = strlen(str);
    if (len > JAEGERTRACINGC_UINT64_MAX_STR_LEN * 2) {
        return false;
    }
    uint64_t high = 0;
    uint64_t low = 0;
    char* iter = NULL;
    if (len > JAEGERTRACINGC_UINT64_MAX_STR_LEN) {
        const char* low_start = str + len - JAEGERTRACINGC_UINT64_MAX_STR_LEN;
        const int high_len = low_start - str;
        char buffer[high_len + 1];
        memcpy(buffer, str, high_len);
        buffer[high_len] = '\0';
        high = strtoull(buffer, &iter, JAEGERTRACINGC_HEX_BASE);
        assert(iter != NULL);
        if (*iter != '\0') {
            return false;
        }
        iter = NULL;
        str = low_start;
    }
    low = strtoull(str, &iter, JAEGERTRACINGC_HEX_BASE);
    assert(iter != NULL);
    if (*iter != '\0') {
        return false;
    }
    *trace_id = (jaeger_trace_id){.high = high, .low = low};
    return true;
}

#ifdef __cplusplus
} /* extern C */
#endif /* __cplusplus */

#endif /* JAEGERTRACINGC_TRACE_ID_H */
