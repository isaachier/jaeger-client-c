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

/**
 * Tracer implementation.
 */
typedef struct jaeger_tracer {
    /** Name of the current service. */
    char* service_name;

    /**
     * Sampler to select spans for tracing.
     * @see jaeger_sampler
     */
    jaeger_sampler* sampler;

    /**
     * Reporter to handle spans after they are completed.
     * @see jaeger_reporter
     */
    jaeger_reporter* reporter;

    /**
     * Custom options passed into the tracer at construction.
     * @see jaeger_tracer_options
     * @see jaeger_tracer_init()
     */
    jaeger_tracer_options options;

    /**
     * Tags to store metadata about the current process (i.e. hostname,
     * client version, etc.).
     */
    jaeger_vector tags;
} jaeger_tracer;

/**
 * Static initializer for jaeger_tracer. May be used to initialize an empty
 * tracer, but the caller must also invoke jaeger_tracer_init to fully construct
 * the tracer.
 * @see jaeger_tracer_init
 */
#define JAEGERTRACINGC_TRACER_INIT                               \
    {                                                            \
        .service_name = NULL, .sampler = NULL, .reporter = NULL, \
        .options = JAEGER_TRACER_OPTIONS_INIT, .tags = NULL      \
    }

/**
 * Initialize a new tracer.
 * @param tracer Tracer to initialize.
 * @param sampler Sampler for tracer to use.
 * @param reporter Reporter for tracer to use.
 * @param options Options for tracer to use, may be NULL.
 * @return True on success, false otherwise.
 */
bool jaeger_tracer_init(jaeger_tracer* tracer,
                        jaeger_sampler* sampler,
                        jaeger_reporter* reporter,
                        const jaeger_tracer_options* options);

/**
 * @internal
 * Report completed span.
 * @param tracer Tracer instance.
 * @param span Span to report.
 */
void jaeger_tracer_report_span(jaeger_tracer* tracer, jaeger_span* span);

#ifdef __cplusplus
} /* extern C */
#endif /* __cplusplus */

#endif /* JAEGERTRACINGC_TRACER_H */
