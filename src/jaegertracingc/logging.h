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

#ifndef JAEGERTRACINGC_LOGGING_H
#define JAEGERTRACINGC_LOGGING_H

#include "jaegertracingc/common.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct jaeger_logger {
    void (*error)(struct jaeger_logger* logger, const char* format, ...);
    void (*warn)(struct jaeger_logger* logger, const char* format, ...);
    void (*info)(struct jaeger_logger* logger, const char* format, ...);
} jaeger_logger;

void jaeger_std_logger_init(jaeger_logger* logger);

/* Shared instance of null logger. DO NOT MODIFY MEMBERS! */
jaeger_logger* jaeger_null_logger();

jaeger_logger* jaeger_default_logger();

#ifdef __cplusplus
} /* extern C */
#endif /* __cplusplus */

#endif /* JAEGERTRACINGC_LOGGING_H */
