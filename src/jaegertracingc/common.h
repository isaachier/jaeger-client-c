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

/**
 * @file
 * Common headers, struct definitions, and macros.
 */

#ifndef JAEGERTRACINGC_COMMON_H
#define JAEGERTRACINGC_COMMON_H

#include "jaegertracingc/config.h"

#include <assert.h>
#include <ctype.h>
#include <inttypes.h>
#include <limits.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <opentracing-c/tracer.h>

#include "jaegertracingc/alloc.h"
#include "jaegertracingc/logging.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define JAEGERTRACINGC_MIN(a, b) ((b) < (a) ? (b) : (a))

#define JAEGERTRACINGC_MAX(a, b) ((b) > (a) ? (b) : (a))

#define JAEGERTRACINGC_CLAMP(x, low, high) \
    JAEGERTRACINGC_MIN(JAEGERTRACINGC_MAX((x), (low)), (high))

#define JAEGERTRACINGC_WRAP_COPY(copy, dst_type, src_type)       \
    static inline bool copy##_wrapper(                           \
        void* arg, void* restrict dst, const void* restrict src) \
    {                                                            \
        (void) arg;                                              \
        return copy((dst_type*) dst, (src_type*) src);           \
    }

#define JAEGERTRACINGC_WRAP_DESTROY(destroy, type) \
    static inline void destroy##_wrapper(void* x)  \
    {                                              \
        destroy((type*) x);                        \
    }

typedef opentracing_destructible jaeger_destructible;

void jaeger_destructible_destroy(jaeger_destructible* d);

void jaeger_destructible_destroy_wrapper(void* d);

#ifdef __cplusplus
} /* extern C */
#endif /* __cplusplus */

#endif /* JAEGERTRACINGC_COMMON_H */
