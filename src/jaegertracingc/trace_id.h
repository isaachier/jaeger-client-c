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
 * Trace ID representation.
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

/** Max string length to represent uint64_t as hex (not including null byte). */
#define JAEGERTRACINGC_UINT64_MAX_STR_LEN 16
/** Max string length to represent trace ID (not including null byte). */
#define JAEGERTRACINGC_TRACE_ID_MAX_STR_LEN \
    (JAEGERTRACINGC_UINT64_MAX_STR_LEN * 2)
/** Base number for hexadecimal (16). */
#define JAEGERTRACINGC_HEX_BASE 16

/**
 * Convert a trace ID to its Protobuf-C representation.
 * @param dst Protobuf-C output argument.
 * @param src jaeger_trace_id source argument.
 */
void jaeger_trace_id_to_protobuf(Jaegertracing__Protobuf__TraceID* dst,
                                 const jaeger_trace_id* src);

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
int jaeger_trace_id_format(const jaeger_trace_id* trace_id,
                           char* buffer,
                           int buffer_len);

/**
 * Scan a string representation of a trace ID into a trace ID instance.
 * @param trace_id The output trace ID.
 * @param str The input string representation.
 * @return True on success, false otherwise.
 * @see jaeger_trace_id_format()
 */
bool jaeger_trace_id_scan(jaeger_trace_id* trace_id, const char* str);

#ifdef __cplusplus
} /* extern C */
#endif /* __cplusplus */

#endif /* JAEGERTRACINGC_TRACE_ID_H */
