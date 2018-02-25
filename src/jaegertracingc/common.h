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

#ifndef JAEGERTRACINGC_COMMON_H
#define JAEGERTRACINGC_COMMON_H

#include <assert.h>
#include <ctype.h>
#include <inttypes.h>
#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define JAEGERTRACINGC_MIN(a, b) ((b) < (a) ? (b) : (a))

#define JAEGERTRACINGC_MAX(a, b) ((b) > (a) ? (b) : (a))

#define JAEGERTRACINGC_CLAMP(x, low, high) \
    JAEGERTRACINGC_MIN(JAEGERTRACINGC_MAX((x), (low)), (high))

#define JAEGERTRACINGC_DESTRUCTIBLE_SUBCLASS \
    void (*destroy)(struct jaeger_destructible*)

typedef struct jaeger_destructible {
    JAEGERTRACINGC_DESTRUCTIBLE_SUBCLASS;
} jaeger_destructible;

static inline void jaeger_destructible_destroy(jaeger_destructible* d)
{
    if (d == NULL) {
        return;
    }
    d->destroy(d);
}

static inline void jaeger_destructible_destroy_wrapper(void* d)
{
    jaeger_destructible_destroy((jaeger_destructible*) d);
}

#ifdef __cplusplus
} /* extern C */
#endif /* __cplusplus */

#endif /* JAEGERTRACINGC_COMMON_H */
