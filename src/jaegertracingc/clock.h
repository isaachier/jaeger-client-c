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

#ifndef JAEGERTRACINGC_CLOCK_H
#define JAEGERTRACINGC_CLOCK_H

#include "jaegertracingc/common.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define JAEGERTRACINGC_NANOSECONDS_PER_SECOND 1000000000
#define JAEGERTRACINGC_MICROSECONDS_PER_SECOND 1000000
#define JAEGERTRACINGC_NANOSECONDS_PER_MICROSECOND 1000

typedef struct timespec jaeger_timestamp;

#define JAEGERTRACINGC_TIMESTAMP_INIT \
    {                                 \
        .tv_sec = 0, .tv_nsec = 0     \
    }

static inline void jaeger_timestamp_now(jaeger_timestamp* timestamp)
{
    assert(timestamp != NULL);
    clock_gettime(CLOCK_REALTIME, timestamp);
}

static inline int64_t
jaeger_timestamp_microseconds(const jaeger_timestamp* const timestamp)
{
    assert(timestamp != NULL);
    return timestamp->tv_sec * JAEGERTRACINGC_MICROSECONDS_PER_SECOND +
           timestamp->tv_nsec * JAEGERTRACINGC_NANOSECONDS_PER_MICROSECOND;
}

typedef struct timespec jaeger_duration;

#define JAEGERTRACINGC_DURATION_INIT JAEGERTRACINGC_TIMESTAMP_INIT

static inline void jaeger_duration_now(jaeger_duration* duration)
{
    assert(duration != NULL);
    clock_gettime(CLOCK_MONOTONIC, duration);
}

// Algorithm based on
// http://www.gnu.org/software/libc/manual/html_node/Elapsed-Time.html.
static inline bool
jaeger_duration_subtract(const jaeger_duration* restrict const lhs,
                         const jaeger_duration* restrict const rhs,
                         jaeger_duration* restrict result)
{
    assert(lhs != NULL);
    assert(rhs != NULL);
    assert(result != NULL);

    jaeger_duration x = *lhs;
    jaeger_duration y = *rhs;

    if (x.tv_nsec < y.tv_nsec) {
        const int64_t nsec =
            (y.tv_nsec - x.tv_nsec) / JAEGERTRACINGC_NANOSECONDS_PER_SECOND + 1;
        y.tv_nsec -= JAEGERTRACINGC_NANOSECONDS_PER_SECOND * nsec;
        y.tv_sec += nsec;
    }

    if (x.tv_nsec - y.tv_nsec > JAEGERTRACINGC_NANOSECONDS_PER_SECOND) {
        const int64_t nsec =
            (x.tv_nsec - y.tv_nsec) / JAEGERTRACINGC_NANOSECONDS_PER_SECOND;
        y.tv_nsec += JAEGERTRACINGC_NANOSECONDS_PER_SECOND * nsec;
        y.tv_sec -= nsec;
    }

    result->tv_sec = x.tv_sec - y.tv_sec;
    result->tv_nsec = x.tv_nsec - y.tv_nsec;
    return x.tv_sec >= y.tv_sec;
}

#ifdef __cplusplus
} /* extern C */
#endif /* __cplusplus */

#endif /* JAEGERTRACINGC_CLOCK_H */
