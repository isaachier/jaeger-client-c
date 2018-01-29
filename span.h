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

#include <stddef.h>
#include <sds.h>

struct jaegertracing_tracer;

#define DEFINE_LIST(inner) \
    typedef struct jaegertracing_##inner##_list { \
        jaegertracing_##inner data; \
        struct jaegertracing_##inner##_list* next; \
    } jaegertracing_##inner##_list

typedef struct jaegertracing_key_value {
    sds key;
    sds value;
} jaegertracing_key_value;

DEFINE_LIST(key_value);

typedef struct jaegertracing_span_context {
    uint64_t trace_id[2];  /* trace_id[0] = hi, trace_id[1] = lo */
    uint64_t span_id;
    uint64_t parent_id;
    uint8_t flags;
    jaegertracing_key_value_list* baggage;
    sds debug_id;
} jaegertracing_span_context;

typedef enum jaegertracing_span_reference_type {
    jaegertracing_child_of_ref,
    jaegertracing_follows_from_ref
} jaegertracing_span_reference_type;

typedef struct jaegertracing_span_reference {
    const jaegertracing_span_context* context;
    jaegertracing_span_reference_type type;
} jaegertracing_span_reference;

DEFINE_LIST(span_reference);

typedef struct jaegertracing_span {
    struct jaegertracing_tracer* tracer;
    jaegertracing_span_context context;
    sds operation_name;
    struct timeval start_time_system;
    struct timespec start_time_monotonic;
    jaegertracing_key_value_list* tags;
    jaegertracing_span_reference_list* references;
} jaegertracing_span;

#undef DEFINE_LIST

#endif  /* JAEGERTRACINGC_SPAN_H */
