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

#include <stdarg.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#ifdef JAEGERTRACINGC_HAVE_FORMAT_ATTRIBUTE
#define JAEGERTRACINGC_FORMAT_ATTRIBUTE(     \
    archetype, string_index, first_to_check) \
    __attribute__((format(archetype, string_index, first_to_check)))
#else
#define JAEGERTRACINGC_FORMAT_ATTRIBUTE(archetype, string_index, first_to_check)
#endif

#define JAEGERTRACINGC_LOGGER_SUBCLASS                                    \
    /** Error log function.                                               \
     *  @param logger Logger instance.                                    \
     *  @param format Format string of the message.                       \
     *  @param[in] ... Arguments to format.                               \
     */                                                                   \
    void (*error)(                                                        \
        struct jaeger_logger * logger, const char* format, va_list args); \
                                                                          \
    /** Warn log function.                                                \
     *  @param logger Logger instance.                                    \
     *  @param format Format string of the message.                       \
     *  @param[in] ... Arguments to format.                               \
     */                                                                   \
    void (*warn)(                                                         \
        struct jaeger_logger * logger, const char* format, va_list args); \
                                                                          \
    /** Info log function.                                                \
     *  @param logger Logger instance.                                    \
     *  @param format Format string of the message.                       \
     *  @param[in] ... Arguments to format.                               \
     */                                                                   \
    void (*info)(                                                         \
        struct jaeger_logger * logger, const char* format, va_list args);

/** Logger interface to customize log output. */
typedef struct jaeger_logger {
    JAEGERTRACINGC_LOGGER_SUBCLASS;
} jaeger_logger;

/**
 * Initialze a logger that prints stdout/stderr. Error and warn functions
 * print to stderr. Info function prints to stdout. Logger locks to avoid
 * races in multithreaded environments.
 * @param logger Uninitialized logger.
 */
void jaeger_std_logger_init(jaeger_logger* logger);

/**
 * Shared instance of null logger. All methods are no-ops.
 * DO NOT MODIFY MEMBERS!
 */
jaeger_logger* jaeger_null_logger();

/**
 * Install shared logger instance.
 * @param Logger to install.
 */
void jaeger_set_logger(jaeger_logger* logger);

/**
 * Get instance of installed logger. DO NOT MODIFY MEMBERS!
 * @return Installed logger instance.
 */
jaeger_logger* jaeger_get_logger(void);

/**
 * Log error to installed logger.
 * @param format Format string.
 * @param[in] ... Arguments to format.
 */
static inline void jaeger_log_error(const char* format, ...)
    JAEGERTRACINGC_FORMAT_ATTRIBUTE(printf, 1, 2);

static inline void jaeger_log_error(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    jaeger_logger* logger = jaeger_get_logger();
    logger->error(logger, format, args);
    va_end(args);
}

/**
 * Log warning to installed logger.
 * @param format Format string.
 * @param[in] ... Arguments to format.
 */
static inline void jaeger_log_warn(const char* format, ...)
    JAEGERTRACINGC_FORMAT_ATTRIBUTE(printf, 1, 2);

static inline void jaeger_log_warn(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    jaeger_logger* logger = jaeger_get_logger();
    logger->warn(logger, format, args);
    va_end(args);
}

/**
 * Log info to installed logger.
 * @param format Format string.
 * @param[in] ... Arguments to format.
 */
static inline void jaeger_log_info(const char* format, ...)
    JAEGERTRACINGC_FORMAT_ATTRIBUTE(printf, 1, 2);

static inline void jaeger_log_info(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    jaeger_logger* logger = jaeger_get_logger();
    logger->info(logger, format, args);
    va_end(args);
}

#ifdef __cplusplus
} /* extern C */
#endif /* __cplusplus */

#endif /* JAEGERTRACINGC_LOGGING_H */
