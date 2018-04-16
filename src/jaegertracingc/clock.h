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

static inline void jaeger_timestamp_now(jaeger_timestamp* timestamp)
{
    assert(timestamp != NULL);
    struct timespec t;
    clock_gettime(CLOCK_REALTIME, &t);
    timestamp->value.tv_sec = t.tv_sec;
    timestamp->value.tv_nsec = t.tv_nsec;
}

static inline int64_t
jaeger_timestamp_microseconds(const jaeger_timestamp* const timestamp)
{
    assert(timestamp != NULL);
    return timestamp->value.tv_sec * JAEGERTRACINGC_MICROSECONDS_PER_SECOND +
           timestamp->value.tv_nsec *
               JAEGERTRACINGC_NANOSECONDS_PER_MICROSECOND;
}

#define JAEGERTRACINGC_DURATION_INIT JAEGERTRACINGC_TIMESTAMP_INIT

static inline void jaeger_duration_now(jaeger_duration* duration)
{
    assert(duration != NULL);
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    duration->value.tv_sec = t.tv_sec;
    duration->value.tv_nsec = t.tv_nsec;
}

// Algorithm based on
// http://www.gnu.org/software/libc/manual/html_node/Elapsed-Time.html.
static inline bool jaeger_time_subtract(opentracing_time_value lhs,
                                        opentracing_time_value rhs,
                                        opentracing_time_value* result)
{
    assert(result != NULL);

    if (lhs.tv_nsec < rhs.tv_nsec) {
        const int64_t nsec = (rhs.tv_nsec - lhs.tv_nsec) /
                                 JAEGERTRACINGC_NANOSECONDS_PER_SECOND +
                             1;
        rhs.tv_nsec -= JAEGERTRACINGC_NANOSECONDS_PER_SECOND * nsec;
        rhs.tv_sec += nsec;
    }

    if (lhs.tv_nsec - rhs.tv_nsec > JAEGERTRACINGC_NANOSECONDS_PER_SECOND) {
        const int64_t nsec =
            (lhs.tv_nsec - rhs.tv_nsec) / JAEGERTRACINGC_NANOSECONDS_PER_SECOND;
        rhs.tv_nsec += JAEGERTRACINGC_NANOSECONDS_PER_SECOND * nsec;
        rhs.tv_sec -= nsec;
    }

    result->tv_sec = lhs.tv_sec - rhs.tv_sec;
    result->tv_nsec = lhs.tv_nsec - rhs.tv_nsec;
    return lhs.tv_sec >= rhs.tv_sec;
}

#ifdef __cplusplus
} /* extern C */
#endif /* __cplusplus */

#endif /* JAEGERTRACINGC_CLOCK_H */
