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

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define JAEGERTRACINGC_REPORTER_SUBCLASS  \
    JAEGERTRACINGC_DESTRUCTIBLE_SUBCLASS; \
    void (*report)(const jaeger_span* span)

typedef struct jaeger_reporter {
    JAEGERTRACINGC_REPORTER_SUBCLASS;
} jaeger_reporter;

typedef struct jaeger_logging_reporter {
    JAEGERTRACINGC_REPORTER_SUBCLASS;
} jaeger_logger_reporter;

#ifdef __cplusplus
} /* extern C */
#endif /* __cplusplus */

#endif /* JAEGERTRACINGC_REPORTER_H */
