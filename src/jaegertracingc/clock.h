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
 * Timestamp and duration representations.
 */

#ifndef JAEGERTRACINGC_CLOCK_H
#define JAEGERTRACINGC_CLOCK_H

#include "jaegertracingc/common.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define JAEGERTRACINGC_NANOSECONDS_PER_SECOND 1000000000
#define JAEGERTRACINGC_MICROSECONDS_PER_SECOND 1000000
#define JAEGERTRACINGC_NANOSECONDS_PER_MICROSECOND 1000

typedef opentracing_duration jaeger_duration;
typedef opentracing_timestamp jaeger_timestamp;

#ifdef OPENTRACINGC_USE_TIMESPEC

#define JAEGERTRACINGC_TIME_CAST(x, type) (x)

#else

#define JAEGERTRACINGC_TIME_CAST(x, type)            \
    (type)                                           \
    {                                                \
        .tv_sec = (x).tv_sec, .tv_nsec = (x).tv_nsec \
    }

#endif /* OPENTRACINGC_USE_TIMESPEC */

#define JAEGERTRACINGC_TIMESTAMP_INIT         \
    {                                         \
        .value = {.tv_sec = 0, .tv_nsec = 0 } \
    }

void jaeger_timestamp_now(jaeger_timestamp* timestamp);

int64_t jaeger_timestamp_microseconds(const jaeger_timestamp* const timestamp);

#define JAEGERTRACINGC_DURATION_INIT JAEGERTRACINGC_TIMESTAMP_INIT

void jaeger_duration_now(jaeger_duration* duration);

// Algorithm based on
// http://www.gnu.org/software/libc/manual/html_node/Elapsed-Time.html.
bool jaeger_time_subtract(opentracing_time_value lhs,
                          opentracing_time_value rhs,
                          opentracing_time_value* result);

#ifdef __cplusplus
} /* extern C */
#endif /* __cplusplus */

#endif /* JAEGERTRACINGC_CLOCK_H */
