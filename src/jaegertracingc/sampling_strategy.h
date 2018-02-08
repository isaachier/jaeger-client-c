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

#ifndef JAEGERTRACINGC_SAMPLING_STRATEGY_H
#define JAEGERTRACINGC_SAMPLING_STRATEGY_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "jaegertracingc/alloc.h"
#include "jaegertracingc/common.h"
#include "jaegertracingc/protoc-gen/sampling.pb-c.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef Jaegertracing__Protobuf__SamplingManager__PerOperationSamplingStrategy
    jaeger_per_operation_strategy;

#define JAEGERTRACINGC_PER_OPERATION_STRATEGY_INIT \
    JAEGERTRACING__PROTOBUF__SAMPLING_MANAGER__PER_OPERATION_SAMPLING_STRATEGY__INIT

typedef Jaegertracing__Protobuf__SamplingManager__PerOperationSamplingStrategy__OperationSamplingStrategy
    jaeger_operation_strategy;

#define JAEGERTRACINGC_OPERATION_STRATEGY_INIT \
    JAEGERTRACING__PROTOBUF__SAMPLING_MANAGER__PER_OPERATION_SAMPLING_STRATEGY__OPERATION_SAMPLING_STRATEGY__INIT

#define JAEGERTRACINGC_OPERATION_STRATEGY_TYPE(type) \
    JAEGERTRACING__PROTOBUF__SAMPLING_MANAGER__PER_OPERATION_SAMPLING_STRATEGY__OPERATION_SAMPLING_STRATEGY__STRATEGY_##type

typedef Jaegertracing__Protobuf__SamplingManager__ProbabilisticSamplingStrategy
    jaeger_probabilistic_strategy;

#define JAEGERTRACINGC_PROBABILISTIC_STRATEGY_INIT \
    JAEGERTRACING__PROTOBUF__SAMPLING_MANAGER__PROBABILISTIC_SAMPLING_STRATEGY__INIT

typedef Jaegertracing__Protobuf__SamplingManager__RateLimitingSamplingStrategy
    jaeger_rate_limiting_strategy;

#define JAEGERTRACINGC_RATE_LIMITING_STRATEGY_INIT \
    JAEGERTRACING__PROTOBUF__SAMPLING_MANAGER__RATE_LIMITING_SAMPLING_STRATEGY__INIT

typedef Jaegertracing__Protobuf__SamplingManager__SamplingStrategyResponse
    jaeger_strategy_response;

#define JAEGERTRACINGC_STRATEGY_RESPONSE_INIT \
    JAEGERTRACING__PROTOBUF__SAMPLING_MANAGER__SAMPLING_STRATEGY_RESPONSE__INIT

#define JAEGERTRACINGC_STRATEGY_RESPONSE_TYPE(x) \
    JAEGERTRACING__PROTOBUF__SAMPLING_MANAGER__SAMPLING_STRATEGY_RESPONSE__STRATEGY_##x

static inline void
jaeger_operation_strategy_destroy(jaeger_operation_strategy* strategy)
{
    assert(strategy != NULL);
    if (strategy->operation != NULL) {
        jaeger_free(strategy->operation);
        strategy->operation = NULL;
    }
    if (strategy->probabilistic != NULL) {
        jaeger_free(strategy->probabilistic);
        strategy->probabilistic = NULL;
    }
    if (strategy->rate_limiting != NULL) {
        jaeger_free(strategy->rate_limiting);
        strategy->rate_limiting = NULL;
    }
}

static inline void
jaeger_per_operation_strategy_destroy(jaeger_per_operation_strategy* strategy)
{
    assert(strategy != NULL);
    if (strategy->per_operation_strategy != NULL) {
        for (int i = 0; i < strategy->n_per_operation_strategy; i++) {
            if (strategy->per_operation_strategy[i] != NULL) {
                jaeger_operation_strategy_destroy(
                    strategy->per_operation_strategy[i]);
                jaeger_free(strategy->per_operation_strategy[i]);
            }
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
    case JAEGERTRACINGC_STRATEGY_RESPONSE_TYPE(PER_OPERATION): {
        if (response->per_operation != NULL) {
            jaeger_per_operation_strategy_destroy(response->per_operation);
            jaeger_free(response->per_operation);
            response->per_operation = NULL;
        }
    } break;
    case JAEGERTRACINGC_STRATEGY_RESPONSE_TYPE(PROBABILISTIC): {
        if (response->probabilistic != NULL) {
            jaeger_free(response->probabilistic);
            response->probabilistic = NULL;
        }
    } break;
    case JAEGERTRACINGC_STRATEGY_RESPONSE_TYPE(RATE_LIMITING): {
        if (response->rate_limiting != NULL) {
            jaeger_free(response->rate_limiting);
            response->rate_limiting = NULL;
        }
    } break;
    default:
        break;
    }
}

#ifdef __cplusplus
} /* extern C */
#endif /* __cplusplus */

#endif /* JAEGERTRACINGC_SAMPLING_STRATEGY_H */
