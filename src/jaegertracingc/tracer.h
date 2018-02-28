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
 * Tracer implementation.
 */

#ifndef JAEGERTRACINGC_TRACER_H
#define JAEGERTRACINGC_TRACER_H

#include "jaegertracingc/common.h"
#include "jaegertracingc/reporter.h"
#include "jaegertracingc/sampler.h"
#include "jaegertracingc/span.h"
#include "jaegertracingc/tag.h"
#include "jaegertracingc/vector.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * Options that can be used to customize the tracer.
 */
typedef struct jaeger_tracer_options {
    /**
     * Whether to generate a 128 bit trace ID or leave high bits as zero.
     * @see jaeger_trace_id
     */
    bool gen_128_bit;
} jaeger_tracer_options;

#define JAEGER_TRACER_OPTIONS_INIT \
    {                              \
        .gen_128_bit = false       \
    }

typedef struct jaeger_tracer {
    char* service_name;
    jaeger_sampler* sampler;
    jaeger_reporter* reporter;
    jaeger_tracer_options options;
    jaeger_vector tags;
} jaeger_tracer;

#ifdef __cplusplus
} /* extern C */
#endif /* __cplusplus */

#endif /* JAEGERTRACINGC_TRACER_H */
