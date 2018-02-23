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

#include "jaegertracingc/clock.h"
#include "jaegertracingc/common.h"
#include "jaegertracingc/constants.h"
#include "jaegertracingc/metrics.h"
#include "jaegertracingc/net.h"
#include "jaegertracingc/sampling_strategy.h"
#include "jaegertracingc/tag.h"
#include "jaegertracingc/threading.h"
#include "jaegertracingc/token_bucket.h"
#include "jaegertracingc/trace_id.h"
#include "jaegertracingc/vector.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define JAEGERTRACINGC_DOUBLE_STR_SIZE 16

#define JAEGERTRACINGC_SAMPLER_SUBCLASS                 \
    JAEGERTRACINGC_DESTRUCTIBLE_SUBCLASS;               \
    bool (*is_sampled)(struct jaeger_sampler * sampler, \
                       const jaeger_trace_id* trace_id, \
                       const char* operation,           \
                       jaeger_vector* tags,             \
                       jaeger_logger* logger)

typedef struct jaeger_sampler {
    JAEGERTRACINGC_SAMPLER_SUBCLASS;
} jaeger_sampler;

typedef struct jaeger_const_sampler {
    JAEGERTRACINGC_SAMPLER_SUBCLASS;
    bool decision;
} jaeger_const_sampler;

void jaeger_const_sampler_init(jaeger_const_sampler* sampler, bool decision);

typedef struct jaeger_probabilistic_sampler {
    JAEGERTRACINGC_SAMPLER_SUBCLASS;
    double sampling_rate;
} jaeger_probabilistic_sampler;

void jaeger_probabilistic_sampler_init(jaeger_probabilistic_sampler* sampler,
                                       double sampling_rate);

typedef struct jaeger_rate_limiting_sampler {
    JAEGERTRACINGC_SAMPLER_SUBCLASS;
    jaeger_token_bucket tok;
    double max_traces_per_second;
} jaeger_rate_limiting_sampler;

void jaeger_rate_limiting_sampler_init(jaeger_rate_limiting_sampler* sampler,
                                       double max_traces_per_second);

typedef struct jaeger_guaranteed_throughput_probabilistic_sampler {
    JAEGERTRACINGC_SAMPLER_SUBCLASS;
    jaeger_probabilistic_sampler probabilistic_sampler;
    jaeger_rate_limiting_sampler lower_bound_sampler;
} jaeger_guaranteed_throughput_probabilistic_sampler;

void jaeger_guaranteed_throughput_probabilistic_sampler_init(
    jaeger_guaranteed_throughput_probabilistic_sampler* sampler,
    double lower_bound,
    double sampling_rate);

void jaeger_guaranteed_throughput_probabilistic_sampler_update(
    jaeger_guaranteed_throughput_probabilistic_sampler* sampler,
    double lower_bound,
    double sampling_rate);

/* Used in jaeger_adaptive_sampler, not a new sampler type. */
typedef struct jaeger_operation_sampler {
    char* operation_name;
    jaeger_guaranteed_throughput_probabilistic_sampler sampler;
} jaeger_operation_sampler;

static inline void
jaeger_operation_sampler_destroy(jaeger_operation_sampler* op_sampler)
{
    assert(op_sampler != NULL);
    if (op_sampler->operation_name != NULL) {
        jaeger_free(op_sampler->operation_name);
        op_sampler->operation_name = NULL;
    }
}

typedef struct jaeger_adaptive_sampler {
    JAEGERTRACINGC_SAMPLER_SUBCLASS;
    jaeger_vector op_samplers;
    jaeger_probabilistic_sampler default_sampler;
    double lower_bound;
    int max_operations;
    jaeger_mutex mutex;
} jaeger_adaptive_sampler;

bool jaeger_adaptive_sampler_init(
    jaeger_adaptive_sampler* sampler,
    const jaeger_per_operation_strategy* strategies,
    int max_operations,
    jaeger_logger* logger);

typedef enum jaeger_sampler_type {
    jaeger_const_sampler_type,
    jaeger_probabilistic_sampler_type,
    jaeger_rate_limiting_sampler_type,
    jaeger_guaranteed_throughput_probabilistic_sampler_type,
    jaeger_adaptive_sampler_type
} jaeger_sampler_type;

typedef struct jaeger_sampler_choice {
    jaeger_sampler_type type;
    union {
        jaeger_const_sampler const_sampler;
        jaeger_probabilistic_sampler probabilistic_sampler;
        jaeger_rate_limiting_sampler rate_limiting_sampler;
        jaeger_guaranteed_throughput_probabilistic_sampler
            guaranteed_throughput_probabilistic_sampler;
        jaeger_adaptive_sampler adaptive_sampler;
    };
} jaeger_sampler_choice;

#define JAEGERTRACINGC_SAMPLER_CHOICE_INIT \
    {                                      \
        .type = -1, .const_sampler = {     \
            .is_sampled = NULL,            \
            .destroy = NULL,               \
            .decision = false              \
        }                                  \
    }

static inline jaeger_sampler*
jaeger_sampler_choice_get_sampler(jaeger_sampler_choice* sampler,
                                  jaeger_logger* logger)
{
#define JAEGERTRACINGC_SAMPLER_TYPE_CASE(type) \
    case jaeger_##type##_sampler_type:         \
        return (jaeger_sampler*) &sampler->type##_sampler;

    switch (sampler->type) {
        JAEGERTRACINGC_SAMPLER_TYPE_CASE(const);
        JAEGERTRACINGC_SAMPLER_TYPE_CASE(probabilistic);
        JAEGERTRACINGC_SAMPLER_TYPE_CASE(rate_limiting);
        JAEGERTRACINGC_SAMPLER_TYPE_CASE(guaranteed_throughput_probabilistic);
        JAEGERTRACINGC_SAMPLER_TYPE_CASE(adaptive);
    default:
        logger->warn(
            logger,
            "Invalid sampler type in sampler choice, sampler type = %d",
            sampler->type);
        return NULL;
    }

#undef JAEGERTRACINGC_SAMPLER_TYPE_CASE
}

static inline void jaeger_sampler_choice_destroy(jaeger_sampler_choice* sampler)
{
    assert(sampler != NULL);
    jaeger_sampler* s =
        jaeger_sampler_choice_get_sampler(sampler, jaeger_null_logger());
    if (s != NULL) {
        s->destroy((jaeger_destructible*) s);
    }
}

#define JAEGERTRACINGC_HTTP_SAMPLING_MANAGER_REQUEST_MAX_LEN 256

typedef struct jaeger_http_sampling_manager {
    char* service_name;
    jaeger_url sampling_server_url;
    int fd;
    http_parser parser;
    http_parser_settings settings;
    int request_length;
    char request_buffer[JAEGERTRACINGC_HTTP_SAMPLING_MANAGER_REQUEST_MAX_LEN];
    jaeger_vector response;
} jaeger_http_sampling_manager;

#define JAEGERTRACINGC_HTTP_SAMPLING_MANAGER_INIT                             \
    {                                                                         \
        .service_name = NULL, .sampling_server_url = JAEGERTRACINGC_URL_INIT, \
        .fd = -1, .parser = {}, .settings = {}, .request_length = 0,          \
        .request_buffer = {'\0'}, .response = JAEGERTRACINGC_VECTOR_INIT      \
    }

typedef struct jaeger_remotely_controlled_sampler {
    JAEGERTRACINGC_SAMPLER_SUBCLASS;
    jaeger_sampler_choice sampler;
    int max_operations;
    jaeger_metrics* metrics;
    jaeger_http_sampling_manager manager;
    jaeger_mutex mutex;
} jaeger_remotely_controlled_sampler;

bool jaeger_remotely_controlled_sampler_init(
    jaeger_remotely_controlled_sampler* sampler,
    const char* service_name,
    const char* sampling_server_url,
    const jaeger_sampler_choice* initial_sampler,
    int max_operations,
    jaeger_metrics* metrics,
    jaeger_logger* logger);

bool jaeger_remotely_controlled_sampler_update(
    jaeger_remotely_controlled_sampler* sampler, jaeger_logger* logger);

#ifdef __cplusplus
} /* extern C */
#endif /* __cplusplus */

#endif /* JAEGERTRACINGC_SAMPLER_H */
