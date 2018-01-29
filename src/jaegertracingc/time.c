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

#include "jaegertracingc/time.h"

#include <assert.h>

#define NS_PER_S JAEGERTRACINGC_NANOSECONDS_PER_SECOND

void jaeger_duration_now(jaeger_duration* duration)
{
    assert(duration != NULL);
    clock_gettime(CLOCK_MONOTONIC, duration);
}

// Algorithm based on
// http://www.gnu.org/software/libc/manual/html_node/Elapsed-Time.html.
int jaeger_duration_subtract(const jaeger_duration* lhs,
                             const jaeger_duration* rhs,
                             jaeger_duration* result)
{
    assert(lhs != NULL);
    assert(rhs != NULL);
    assert(result != NULL);

    jaeger_duration x = *lhs;
    jaeger_duration y = *rhs;

    if (x.tv_nsec < y.tv_nsec) {
        const int64_t nsec = (y.tv_nsec - x.tv_nsec) / NS_PER_S + 1;
        y.tv_nsec -= NS_PER_S * nsec;
        y.tv_sec += nsec;
    }

    if (x.tv_nsec - y.tv_nsec > NS_PER_S) {
        const int64_t nsec = (x.tv_nsec - y.tv_nsec) / NS_PER_S;
        y.tv_nsec += NS_PER_S * nsec;
        y.tv_sec -= nsec;
    }

    result->tv_sec = x.tv_sec - y.tv_sec;
    result->tv_nsec = x.tv_nsec - y.tv_nsec;
    return x.tv_sec < y.tv_sec;
}
