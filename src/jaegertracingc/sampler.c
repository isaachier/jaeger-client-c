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

#include "jaegertracingc/sampler.h"

#include <stdio.h>
#include <stdlib.h>

#include "jaegertracingc/alloc.h"
#include "jaegertracingc/constants.h"

typedef struct jaeger_const_sampler {
    JAEGERTRACINGC_SAMPLER_SUBCLASS;
    bool decision;
} jaeger_const_sampler;

static bool const_is_sampled(jaeger_sampler* sampler,
                             const jaeger_trace_id* trace_id,
                             const sds operation_name,
                             jaeger_key_value_list* tags)
{
    (void)trace_id;
    (void)operation_name;
    jaeger_const_sampler* s = (jaeger_const_sampler*)sampler;
    if (tags != NULL) {
        tags->key = sdsnew(JAEGERTRACINGC_SAMPLER_TYPE_TAG_KEY);
        tags->value = sdsnew(JAEGERTRACINGC_SAMPLER_TYPE_CONST);
        tags->next = malloc(sizeof(jaeger_key_value_list));
        tags = tags->next;
        tags->key = sdsnew(JAEGERTRACINGC_SAMPLER_PARAM_TAG_KEY);
        tags->value = sdsnew(s->decision ? "true" : "false");
    }
    return s->decision;
}

static void noop_close(jaeger_sampler* sampler) { (void)sampler; }

jaeger_sampler* jaeger_const_sampler_init(bool decision)
{
    jaeger_const_sampler* sampler =
        jaeger_global_alloc_malloc(sizeof(*sampler));
    if (sampler == NULL) {
        fprintf(stderr, "ERROR: Cannot allocate jaeger_const_sampler\n");
        return NULL;
    }
    sampler->is_sampled = &const_is_sampled;
    sampler->close = &noop_close;
    return (jaeger_sampler*)sampler;
}

typedef struct jaeger_probabilistic_sampler {
    JAEGERTRACINGC_SAMPLER_SUBCLASS;
    double probability;
    unsigned int seed;
} jaeger_probabilistic_sampler;

static bool probabilistic_is_sampled(jaeger_sampler* sampler,
                                     const jaeger_trace_id* trace_id,
                                     const sds operation_name,
                                     jaeger_key_value_list* tags)
{
    jaeger_probabilistic_sampler* s = (jaeger_probabilistic_sampler*)sampler;
    const double threshold = ((double)rand_r(&s->seed)) / RAND_MAX;
    const bool decision = (s->probability >= threshold);
    if (tags != NULL) {
        tags->key = sdsnew(JAEGERTRACINGC_SAMPLER_TYPE_TAG_KEY);
        tags->value = sdsnew(JAEGERTRACINGC_SAMPLER_TYPE_CONST);
        tags->next = malloc(sizeof(jaeger_key_value_list));
        tags = tags->next;
        tags->key = sdsnew(JAEGERTRACINGC_SAMPLER_PARAM_TAG_KEY);
        char buffer[16];
        snprintf(&buffer[0], sizeof(buffer), "%f", s->probability);
        tags->value = sdsnew(&buffer[0]);
    }
    return decision;
}

jaeger_sampler* jaeger_probabilistic_sampler_init(double probability)
{
    jaeger_probabilistic_sampler* sampler =
        jaeger_global_alloc_malloc(sizeof(*sampler));
    if (sampler == NULL) {
        fprintf(stderr,
                "ERROR: Cannot allocate jaeger_probabilistic_sampler\n");
        return NULL;
    }
    sampler->is_sampled = &probabilistic_is_sampled;
    sampler->close = &noop_close;
    return (jaeger_sampler*)sampler;
}

typedef struct jaeger_rate_limiting_sampler {
    JAEGERTRACINGC_SAMPLER_SUBCLASS;
    jaeger_token_bucket* tok;
} jaeger_rate_limiting_sampler;

jaeger_sampler* jaeger_rate_limiting_sampler_init(double max_traces_per_second);

typedef struct jaeger_guaranteed_throughput_probabilistic_sampler {
    JAEGERTRACINGC_SAMPLER_SUBCLASS;
    jaeger_probabilistic_sampler probabilistic_sampler;
    jaeger_rate_limiting_sampler lower_bound_sampler;
} jaeger_guaranteed_throughput_probabilistic_sampler;

jaeger_sampler*
jaeger_guaranteed_throughput_probabilistic_sampler_init(double lower_bound,
                                                        double sampling_rate);

typedef struct jaeger_remotely_controlled_sampler {
    JAEGERTRACINGC_SAMPLER_SUBCLASS;
    sds service_name;
    sds sampling_server_url;
    jaeger_sampler* sampler;
    int max_operations;
    jaeger_duration sampling_refresh_interval;
    jaeger_logger* logger;
} jaeger_remotely_controlled_sampler;

jaeger_sampler* jaeger_remotely_controlled_sampler_init(
    sds service_name,
    jaeger_sampler* initial_sampler,
    int max_operations,
    const jaeger_duration* sampling_refresh_interval,
    jaeger_logger* logger,
    jaeger_metrics* metrics);
