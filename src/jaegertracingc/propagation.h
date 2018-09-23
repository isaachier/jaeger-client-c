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
 * Span context propagation functions and types.
 */

#ifndef JAEGERTRACINGC_PROPAGATION_H
#define JAEGERTRACINGC_PROPAGATION_H

#include <opentracing-c/propagation.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct jaeger_headers_config;
struct jaeger_metrics;
struct jaeger_span_context;
struct jaeger_tracer;

opentracing_propagation_error_code
jaeger_extract_from_text_map(opentracing_text_map_reader* reader,
                             struct jaeger_span_context** ctx,
                             struct jaeger_metrics* metrics,
                             const struct jaeger_headers_config* config);

opentracing_propagation_error_code
jaeger_extract_from_http_headers(opentracing_http_headers_reader* reader,
                                 struct jaeger_span_context** ctx,
                                 struct jaeger_metrics* metrics,
                                 const struct jaeger_headers_config* config);

opentracing_propagation_error_code
jaeger_extract_from_binary(int (*callback)(void*, char*, size_t),
                           void* arg,
                           struct jaeger_span_context** ctx,
                           struct jaeger_metrics* metrics);

opentracing_propagation_error_code
jaeger_extract_from_custom(opentracing_custom_carrier_reader* reader,
                           struct jaeger_tracer* tracer,
                           struct jaeger_span_context** ctx,
                           struct jaeger_metrics* metrics);

opentracing_propagation_error_code
jaeger_inject_into_text_map(opentracing_text_map_writer* writer,
                            const struct jaeger_span_context* ctx,
                            const struct jaeger_headers_config* config);

opentracing_propagation_error_code
jaeger_inject_into_http_headers(opentracing_http_headers_writer* writer,
                                const struct jaeger_span_context* ctx,
                                const struct jaeger_headers_config* config);

opentracing_propagation_error_code
jaeger_inject_into_binary(int (*callback)(void*, const char*, size_t),
                          void* arg,
                          const struct jaeger_span_context* ctx);

opentracing_propagation_error_code
jaeger_inject_into_custom(opentracing_custom_carrier_writer* writer,
                          struct jaeger_tracer* tracer,
                          const struct jaeger_span_context* ctx);

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* JAEGERTRACINGC_PROPAGATION_H */
