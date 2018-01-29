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

#ifndef JAEGERTRACINGC_SAMPLER_H
#define JAEGERTRACINGC_SAMPLER_H

#include "jaegertracingc/common.h"

typedef struct jaeger_sampler {
    int (*is_sampled)(const uint64_t trace_id[2], sds operation);
} jaeger_sampler;

#define SAMPLER_SUBCLASS \
    int (*is_sampled)(const uint64_t trace_id[2], sds operation)

typedef struct jaeger_const_sampler {
    SAMPLER_SUBCLASS;
    int decision;
} jaeger_const_sampler;

typedef struct jaeger_probabilistic_sampler {
    SAMPLER_SUBCLASS;
    double probability;
    unsigned int seed;
} jaeger_probabilistic_sampler;

#endif  // JAEGERTRACINGC_SAMPLER_H
