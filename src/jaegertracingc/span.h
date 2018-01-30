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
#include "jaegertracingc/trace_id.h"

struct jaeger_tracer;

#define DEFINE_LIST(inner)                                                     \
    typedef struct jaeger_##inner##_list {                                     \
        jaeger_##inner data;                                                   \
        struct jaeger_##inner##_list* next;                                    \
    } jaeger_##inner##_list

typedef struct jaeger_key_value {
    sds key;
    sds value;
} jaeger_key_value;

DEFINE_LIST(key_value);

typedef struct jaeger_span_context {
    jaeger_trace_id trace_id;
    uint64_t span_id;
    uint64_t parent_id;
    uint8_t flags;
    jaeger_key_value_list* baggage;
    sds debug_id;
} jaeger_span_context;

typedef enum jaeger_span_reference_type {
    jaeger_child_of_ref,
    jaeger_follows_from_ref
} jaeger_span_reference_type;

typedef struct jaeger_span_reference {
    const jaeger_span_context* context;
    jaeger_span_reference_type type;
} jaeger_span_reference;

DEFINE_LIST(span_reference);

typedef struct jaeger_span {
    struct jaeger_tracer* tracer;
    jaeger_span_context context;
    sds operation_name;
    time_t start_time_system;
    jaeger_duration start_time_monotonic;
    jaeger_key_value_list* tags;
    jaeger_span_reference_list* references;
} jaeger_span;

#undef DEFINE_LIST

#endif /* JAEGERTRACINGC_SPAN_H */
