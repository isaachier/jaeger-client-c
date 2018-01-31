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
#include "jaegertracingc/duration.h"
#include "jaegertracingc/key_value.h"
#include "jaegertracingc/logger.h"
#include "jaegertracingc/metrics.h"
#include "jaegertracingc/token_bucket.h"
#include "jaegertracingc/trace_id.h"

#define JAEGERTRACINGC_SAMPLER_SUBCLASS                                        \
    bool (*is_sampled)(struct jaeger_sampler * sampler,                        \
                       const jaeger_trace_id* trace_id,                        \
                       const sds operation,                                    \
                       jaeger_key_value_list* tags);                           \
    void (*close)(struct jaeger_sampler * sampler)

typedef struct jaeger_sampler {
    JAEGERTRACINGC_SAMPLER_SUBCLASS;
} jaeger_sampler;

typedef struct jaeger_const_sampler {
    JAEGERTRACINGC_SAMPLER_SUBCLASS;
    bool decision;
} jaeger_const_sampler;

typedef struct jaeger_probabilistic_sampler {
    JAEGERTRACINGC_SAMPLER_SUBCLASS;
    double probability;
    unsigned int seed;
} jaeger_probabilistic_sampler;

typedef struct jaeger_rate_limiting_sampler {
    JAEGERTRACINGC_SAMPLER_SUBCLASS;
    jaeger_token_bucket* tok;
} jaeger_rate_limiting_sampler;

typedef struct jaeger_guaranteed_throughput_probabilistic_sampler {
    JAEGERTRACINGC_SAMPLER_SUBCLASS;
    jaeger_probabilistic_sampler probabilistic_sampler;
    jaeger_rate_limiting_sampler lower_bound_sampler;
} jaeger_guaranteed_throughput_probabilistic_sampler;
typedef struct jaeger_remotely_controlled_sampler {
    JAEGERTRACINGC_SAMPLER_SUBCLASS;
    sds service_name;
    sds sampling_server_url;
    jaeger_sampler* sampler;
    int max_operations;
    jaeger_duration sampling_refresh_interval;
    jaeger_logger* logger;
} jaeger_remotely_controlled_sampler;

void jaeger_const_sampler_init(jaeger_const_sampler* sampler, bool decision);

void jaeger_probabilistic_sampler_init(
    jaeger_probabilistic_sampler* sampler, double probability);

void jaeger_rate_limiting_sampler_init(
    jaeger_rate_limiting_sampler* sampler, double max_traces_per_second);

void jaeger_guaranteed_throughput_probabilistic_sampler_init(
    jaeger_guaranteed_throughput_probabilistic_sampler* sampler,
    double lower_bound,
    double sampling_rate);

void jaeger_remotely_controlled_sampler_init(
    jaeger_remotely_controlled_sampler* sampler,
    sds service_name,
    jaeger_sampler* initial_sampler,
    int max_operations,
    const jaeger_duration* sampling_refresh_interval,
    jaeger_logger* logger,
    jaeger_metrics* metrics);

#endif  // JAEGERTRACINGC_SAMPLER_H
