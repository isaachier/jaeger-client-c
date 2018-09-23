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

/**
 * @file
 * Configuration options.
 */

#ifndef JAEGERTRACINGC_OPTIONS_H
#define JAEGERTRACINGC_OPTIONS_H

#include "jaegertracingc/common.h"
#include "jaegertracingc/constants.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * HTTP headers to use for span context propagation.
 */
typedef struct jaeger_headers_config {
    /** Header used to propagate a debug span. */
    const char* debug_header;
    /** Header used to propagate baggage. */
    const char* baggage_header;
    /** Header used to propagate a span context. */
    const char* trace_context_header;
    /** Header prefix to prepend to baggage keys. */
    const char* trace_baggage_header_prefix;
} jaeger_headers_config;

#define JAEGERTRACINGC_HEADERS_CONFIG_INIT                                \
    {                                                                     \
        .debug_header = JAEGERTRACINGC_DEBUG_HEADER,                      \
        .baggage_header = JAEGERTRACINGC_BAGGAGE_HEADER,                  \
        .trace_context_header = JAEGERTRACINGC_TRACE_CONTEXT_HEADER_NAME, \
        .trace_baggage_header_prefix =                                    \
            JAEGERTRACINGC_TRACE_BAGGAGE_HEADER_PREFIX                    \
    }

/** Sampler config. */
typedef struct jaeger_sampler_config {
} jaeger_sampler_config;

/** Reporter config. */
typedef struct jaeger_reporter_config {
} jaeger_reporter_config;

/** Baggage restrictions config. */
typedef struct jaeger_baggage_restrictions_config {
    /* TODO */
} jaeger_baggage_restrictions_config;

/** Overall Jaeger configuration object. */
typedef struct jaeger_config {
    /** Flag to disable tracing. */
    bool disabled;

    /** Sampler config. */
    jaeger_sampler_config sampler;

    /** Reporter config. */
    jaeger_reporter_config reporter;

    /** Headers config. */
    jaeger_headers_config headers;

    /** Baggage restrictions config. */
    jaeger_baggage_restrictions_config baggage_restrictions;
} jaeger_config;

#ifdef __cplusplus
}
#endif

#endif /* JAEGERTRACINGC_OPTIONS_H */
