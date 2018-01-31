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

#define JAEGERTRACINGC_COUNTER_SUBCLASS                                        \
    void (*inc)(struct jaeger_counter * counter, int64_t delta)

typedef struct jaeger_counter {
    JAEGERTRACINGC_COUNTER_SUBCLASS;
} jaeger_counter;

jaeger_counter* jaeger_default_counter_init();

jaeger_counter* jaeger_null_counter_init();

#define JAEGERTRACINGC_GAUGE_SUBCLASS                                          \
    void (*update)(struct jaeger_gauge * gauge, int64_t amount)

typedef struct jaeger_gauge {
    JAEGERTRACINGC_GAUGE_SUBCLASS;
} jaeger_gauge;

jaeger_gauge* jaeger_default_gauge_init();

jaeger_gauge* jaeger_null_gauge_init();

typedef struct jaeger_metrics {
    jaeger_counter* traces_started_sampled;
    jaeger_counter* traces_started_not_sampled;
    jaeger_counter* traces_joined_sampled;
    jaeger_counter* traces_joined_not_sampled;
    jaeger_counter* spans_started;
    jaeger_counter* spans_finished;
    jaeger_counter* spans_sampled;
    jaeger_counter* spans_not_sampled;
    jaeger_counter* decoding_errors;
    jaeger_counter* reporter_success;
    jaeger_counter* reporter_failure;
    jaeger_counter* reporter_dropped;
    jaeger_gauge* reporter_queue_length;
    jaeger_counter* sampler_retrieved;
    jaeger_counter* sampler_updated;
    jaeger_counter* sampler_update_failure;
    jaeger_counter* sampler_query_failure;
    jaeger_counter* sampler_parsing_failure;
    jaeger_counter* baggage_update_success;
    jaeger_counter* baggage_update_failure;
    jaeger_counter* baggage_truncate;
    jaeger_counter* baggage_restrictions_update_success;
    jaeger_counter* baggage_restrictions_update_failure;
} jaeger_metrics;

#endif  // JAEGERTRACINGC_METRICS_H
