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

#ifndef JAEGERTRACINGC_REPORTER_H
#define JAEGERTRACINGC_REPORTER_H

#include "jaegertracingc/common.h"
#include "jaegertracingc/logging.h"
#include "jaegertracingc/span.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define JAEGERTRACINGC_REPORTER_SUBCLASS  \
    JAEGERTRACINGC_DESTRUCTIBLE_SUBCLASS; \
    void (*report)(struct jaeger_reporter * reporter, const jaeger_span* span)

typedef struct jaeger_reporter {
    JAEGERTRACINGC_REPORTER_SUBCLASS;
} jaeger_reporter;

/* Shared instance of null reporter. DO NOT MODIFY MEMBERS! */
jaeger_reporter* jaeger_null_reporter();

typedef struct jaeger_logging_reporter {
    JAEGERTRACINGC_REPORTER_SUBCLASS;
    jaeger_logger* logger;
} jaeger_logging_reporter;

void jaeger_logging_reporter_init(jaeger_logging_reporter* reporter,
                                  jaeger_logger* logger);

typedef struct jaeger_in_memory_reporter {
    JAEGERTRACINGC_REPORTER_SUBCLASS;
    int num_spans;
    int capacity;
    jaeger_span* spans;
} jaeger_in_memory_reporter;

typedef struct jaeger_composite_reporter {
    JAEGERTRACINGC_REPORTER_SUBCLASS;
    int num_reporters;
    int capacity;
    jaeger_reporter* reporters;
} jaeger_composite_reporter;

typedef struct jaeger_remote_reporter {
    JAEGERTRACINGC_REPORTER_SUBCLASS;
    /* TODO */
} jaeger_remote_reporter;

#ifdef __cplusplus
} /* extern C */
#endif /* __cplusplus */

#endif /* JAEGERTRACINGC_REPORTER_H */
