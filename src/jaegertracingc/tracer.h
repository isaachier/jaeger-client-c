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

#include <opentracing-c/tracer.h>

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
    /** Base class instance. */
    opentracing_tracer base;

    /** Name of the current service. */
    char* service_name;

    /**
     * Metrics object to use in tracer.
     * @see jaeger_metrics
     */
    jaeger_metrics* metrics;

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

    /**
     * Flags to keep track of the members that were heap-allocated and must be
     * freed by the tracer.
     */
    struct {
        /** True if metrics is heap-allocated, false otherwise. */
        bool metrics : 1;
        /** True if sampler is heap-allocated, false otherwise. */
        bool sampler : 1;
        /** True if reporter is heap-allocated, false otherwise. */
        bool reporter : 1;
    } allocated;
} jaeger_tracer;

/**
 * Static initializer for jaeger_tracer. May be used to initialize an empty
 * tracer, but the caller must also invoke jaeger_tracer_init to fully construct
 * the tracer.
 * @see jaeger_tracer_init
 */
#define JAEGERTRACINGC_TRACER_INIT                               \
    {                                                            \
        .service_name = NULL, .metrics = NULL, .sampler = NULL,  \
        .reporter = NULL, .options = JAEGER_TRACER_OPTIONS_INIT, \
        .tags = JAEGERTRACINGC_VECTOR_INIT, .allocated = {       \
            .metrics = false,                                    \
            .sampler = false,                                    \
            .reporter = false                                    \
        }                                                        \
    }

/**
 * Close and free resources associated with the tracer.
 * @param tracer Tracer to destroy
 */
void jaeger_tracer_destroy(jaeger_destructible* tracer);

/**
 * Initialize a new tracer.
 * @param tracer Tracer to initialize.
 * @param service_name Name of the current service. May not be NULL.
 *                     Tracer copies this string internally so it need not be
 *                     allocated upfront.
 * @param sampler Sampler for tracer to use. May be NULL.
 * @param reporter Reporter for tracer to use. May be NULL.
 * @param metrics Metrics object to use. May be NULL.
 * @param options Options for tracer to use. May be NULL.
 * @return True on success, false otherwise.
 */
bool jaeger_tracer_init(jaeger_tracer* tracer,
                        const char* service_name,
                        jaeger_sampler* sampler,
                        jaeger_reporter* reporter,
                        jaeger_metrics* metrics,
                        const jaeger_tracer_options* options);

/**
 * Start a new span.
 * @param tracer Tracer instance. May not be NULL.
 * @param operation_name Operation name associated with this span.
 *                       May not be NULL.
 * @param options Additional options for starting span. May be NULL.
 * @return New span on success, NULL otherwise.
 */
jaeger_span*
jaeger_tracer_start_span(opentracing_tracer* tracer,
                         const char* operation_name,
                         const opentracing_start_span_options* options);

/**
 * Flush any pending spans in tracer. Only effective when using remote reporter,
 * which is the default reporter.
 * @param tracer Tracer instance to flush.
 * @return True on success, false otherwise.
 */
bool jaeger_tracer_flush(jaeger_tracer* tracer);

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
