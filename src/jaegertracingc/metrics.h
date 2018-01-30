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

#ifndef JAEGERTRACINGC_METRICS_H
#define JAEGERTRACINGC_METRICS_H

#include "jaegertracingc/common.h"

typedef struct jaeger_counter {
    void (*inc)(jaeger_counter* counter, int64_t delta);
} jaeger_counter;

typedef struct jaeger_gauge {
    void (*update)(jaeger_gauge* gauge, int64_t amount);
} jaeger_gauge;

#endif  // JAEGERTRACINGC_METRICS_H
