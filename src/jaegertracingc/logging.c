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

#include "jaegertracingc/logging.h"
#include "jaegertracingc/threading.h"

static void null_log(jaeger_logger* logger, const char* format, va_list args)
{
    (void) logger;
    (void) format;
    (void) args;
}

static jaeger_mutex stdout_mutex = JAEGERTRACINGC_MUTEX_INIT;
static jaeger_mutex stderr_mutex = JAEGERTRACINGC_MUTEX_INIT;

#define STD_LOG_FUNC(level, stream, mutex)                       \
    static void std_log_##level(                                 \
        jaeger_logger* logger, const char* format, va_list args) \
    {                                                            \
        (void) logger;                                           \
        fprintf((stream), "%s: ", #level);                       \
        jaeger_mutex_lock(&(mutex));                             \
        vfprintf((stream), format, args);                        \
        fputc('\n', (stream));                                   \
        jaeger_mutex_unlock(&(mutex));                           \
    }

jaeger_logger* jaeger_null_logger()
{
    static jaeger_logger null_logger = {
        .error = &null_log, .warn = &null_log, .info = &null_log};
    return &null_logger;
}

STD_LOG_FUNC(error, stderr, stderr_mutex)
STD_LOG_FUNC(warn, stderr, stderr_mutex)
STD_LOG_FUNC(info, stdout, stdout_mutex)

#define STD_LOGGER_INIT                                                       \
    {                                                                         \
        .error = &std_log_error, .warn = &std_log_warn, .info = &std_log_info \
    }

void jaeger_std_logger_init(jaeger_logger* logger)
{
    assert(logger != NULL);
    *logger = (jaeger_logger) STD_LOGGER_INIT;
    logger->error = &std_log_error;
    logger->warn = &std_log_warn;
    logger->info = &std_log_info;
}

jaeger_logger* jaeger_get_logger(void)
{
    static jaeger_logger logger = STD_LOGGER_INIT;
    return &logger;
}

void jaeger_set_logger(jaeger_logger* logger)
{
    assert(logger != NULL);
    *jaeger_get_logger() = *logger;
}

void jaeger_log_error(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    jaeger_logger* logger = jaeger_get_logger();
    logger->error(logger, format, args);
    va_end(args);
}

void jaeger_log_warn(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    jaeger_logger* logger = jaeger_get_logger();
    logger->warn(logger, format, args);
    va_end(args);
}

void jaeger_log_info(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    jaeger_logger* logger = jaeger_get_logger();
    logger->info(logger, format, args);
    va_end(args);
}
