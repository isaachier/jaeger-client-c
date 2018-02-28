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
 * Logging interface and initialization functions.
 */

#ifndef JAEGERTRACINGC_LOGGING_H
#define JAEGERTRACINGC_LOGGING_H

#include "jaegertracingc/common.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/** Logger interface to customize log output. */
typedef struct jaeger_logger {
    /** Error log function.
     *  @param logger The logger instance.
     *  @param format The format string of the message.
     *  @param[in] ... Arguments for format string.
     */
    void (*error)(struct jaeger_logger* logger, const char* format, ...)
        JAEGERTRACINGC_FORMAT_ATTRIBUTE(printf, 2, 3);

    /** Warn log function.
     *  @param logger The logger instance.
     *  @param format The format string of the message.
     *  @param[in] ... Arguments for format string.
     */
    void (*warn)(struct jaeger_logger* logger, const char* format, ...)
        JAEGERTRACINGC_FORMAT_ATTRIBUTE(printf, 2, 3);

    /** Info log function.
     *  @param logger The logger instance.
     *  @param format The format string of the message.
     *  @param[in] ... Arguments for format string.
     */
    void (*info)(struct jaeger_logger* logger, const char* format, ...)
        JAEGERTRACINGC_FORMAT_ATTRIBUTE(printf, 2, 3);
} jaeger_logger;

/**
 * Initialize an uninitialized logger to print stdout/stderr. error and warn
 * print to stderr. info prints to stdout.
 * @param logger The uninitialized logger.
 */
void jaeger_std_logger_init(jaeger_logger* logger);

/** Shared instance of null logger. All methods are no-ops.
 *  DO NOT MODIFY MEMBERS!
 */
jaeger_logger* jaeger_null_logger();

/** Shared instance of installed logger. DO NOT MODIFY MEMBERS!
 *  @see jaeger_init_lib()
 */
jaeger_logger* jaeger_default_logger();

#ifdef __cplusplus
} /* extern C */
#endif /* __cplusplus */

#endif /* JAEGERTRACINGC_LOGGING_H */
