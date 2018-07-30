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
 * Sampling strategy type definitions. Mostly aliases protobuf-c generated
 * types.
 */

#ifndef JAEGERTRACINGC_SAMPLING_STRATEGY_H
#define JAEGERTRACINGC_SAMPLING_STRATEGY_H

#include "jaegertracingc/common.h"
#include "jaegertracingc/protoc-gen/model.pb-c.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef enum jaeger_strategy_type {
    jaeger_probabilistic_strategy_type,
    jaeger_rate_limiting_strategy_type,
    jaeger_per_operation_strategy_type,
} jaeger_strategy_type;

typedef struct jaeger_probabilistic_strategy {
    double sampling_rate;
} jaeger_probabilistic_strategy;

#define JAEGERTRACINGC_PROBABILISTIC_STRATEGY_INIT \
    {                                              \
        .sampling_rate = 0                         \
    }

typedef struct jaeger_rate_limiting_strategy {
    double max_traces_per_second;
} jaeger_rate_limiting_strategy;

#define JAEGERTRACINGC_RATE_LIMITING_STRATEGY_INIT \
    {                                              \
        .max_traces_per_second = 0                 \
    }

typedef struct jaeger_operation_strategy {
    char* operation;
    jaeger_probabilistic_strategy probabilistic;
} jaeger_operation_strategy;

#define JAEGERTRACINGC_OPERATION_STRATEGY_INIT                      \
    {                                                               \
        .operation = NULL,                                          \
        .probabilistic = JAEGERTRACINGC_PROBABILISTIC_STRATEGY_INIT \
    }

typedef struct jaeger_per_operation_strategy {
    jaeger_operation_strategy* per_operation_strategy;
    size_t n_per_operation_strategy;
    double default_sampling_probability;
    double default_lower_bound_traces_per_second;
} jaeger_per_operation_strategy;

#define JAEGERTRACINGC_PER_OPERATION_STRATEGY_INIT                     \
    {                                                                  \
        .per_operation_strategy = NULL, .n_per_operation_strategy = 0, \
        .default_sampling_probability = 0,                             \
        .default_lower_bound_traces_per_second = 0                     \
    }

typedef struct jaeger_strategy_response {
    jaeger_strategy_type strategy_case;
    union {
        jaeger_probabilistic_strategy probabilistic;
        jaeger_rate_limiting_strategy rate_limiting;
        jaeger_per_operation_strategy per_operation;
    } strategy;
} jaeger_strategy_response;

static inline void
jaeger_operation_strategy_destroy(jaeger_operation_strategy* strategy)
{
    assert(strategy != NULL);
    if (strategy->operation != NULL) {
        jaeger_free(strategy->operation);
        strategy->operation = NULL;
    }
}

static inline void
jaeger_per_operation_strategy_destroy(jaeger_per_operation_strategy* strategy)
{
    assert(strategy != NULL);
    if (strategy->per_operation_strategy != NULL) {
        for (size_t i = 0; i < strategy->n_per_operation_strategy; i++) {
            jaeger_operation_strategy_destroy(
                &strategy->per_operation_strategy[i]);
        }
        jaeger_free(strategy->per_operation_strategy);
        strategy->per_operation_strategy = NULL;
    }
}

static inline void
jaeger_strategy_response_destroy(jaeger_strategy_response* response)
{
    assert(response != NULL);
    switch (response->strategy_case) {
    case jaeger_per_operation_strategy_type: {
        jaeger_per_operation_strategy_destroy(
            &response->strategy.per_operation);
    } break;
    default:
        break;
    }
}

#ifdef __cplusplus
} /* extern C */
#endif /* __cplusplus */

#endif /* JAEGERTRACINGC_SAMPLING_STRATEGY_H */
