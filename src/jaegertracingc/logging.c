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

#include "jaegertracingc/logging.h"
#include <stdarg.h>
#include "jaegertracingc/threading.h"

static void null_log(jaeger_logger* logger, const char* format, ...)
{
    (void) logger;
    (void) format;
}

static inline void
std_log(jaeger_logger* logger, const char* format, va_list args)
{
    assert(logger != NULL);
    assert(format != NULL);
    vfprintf(stderr, format, args);
}

#define STD_LOG_FUNC(level, prefix)                               \
    static void std_log_##level(                                  \
        jaeger_logger* logger, const char* format, ...)           \
    {                                                             \
        char fmt[strlen(format) + strlen(#prefix ": ") + 2];      \
        const int result =                                        \
            snprintf(fmt, sizeof(fmt), #prefix ": %s\n", format); \
        (void) result;                                            \
        assert(result < sizeof(fmt));                             \
        va_list args;                                             \
        va_start(args, format);                                   \
        std_log(logger, fmt, args);                               \
        va_end(args);                                             \
    }

static jaeger_logger null_logger;

static void init_null_logger()
{
    null_logger = (jaeger_logger){
        .error = &null_log, .warn = &null_log, .info = &null_log};
}

jaeger_logger* jaeger_null_logger()
{
    static jaeger_once once = JAEGERTRACINGC_ONCE_INIT;
    jaeger_do_once(&once, &init_null_logger);
    return &null_logger;
}

STD_LOG_FUNC(error, ERROR)
STD_LOG_FUNC(warn, WARNING)
STD_LOG_FUNC(info, INFO)

void jaeger_std_logger_init(jaeger_logger* logger)
{
    assert(logger != NULL);
    logger->error = &std_log_error;
    logger->warn = &std_log_warn;
    logger->info = &std_log_info;
}
