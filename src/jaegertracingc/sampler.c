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

#define SAMPLER_GROWTH_RATIO 2

static bool jaeger_const_sampler_is_sampled(jaeger_sampler* sampler,
                                            const jaeger_trace_id* trace_id,
                                            const char* operation_name,
                                            jaeger_tag_list* tags)
{
    (void) trace_id;
    (void) operation_name;
    jaeger_const_sampler* s = (jaeger_const_sampler*) sampler;
    if (tags != NULL) {
        jaeger_tag tag = JAEGERTRACING__PROTOBUF__TAG__INIT;
        tag.key = JAEGERTRACINGC_SAMPLER_TYPE_TAG_KEY;
        tag.value_case = JAEGERTRACING__PROTOBUF__TAG__VALUE_STR_VALUE;
        tag.str_value = JAEGERTRACINGC_SAMPLER_TYPE_CONST;
        jaeger_tag_list_append(tags, &tag);

        tag.key = JAEGERTRACINGC_SAMPLER_PARAM_TAG_KEY;
        tag.value_case = JAEGERTRACING__PROTOBUF__TAG__VALUE_BOOL_VALUE;
        tag.bool_value = s->decision;
        jaeger_tag_list_append(tags, &tag);
    }
    return s->decision;
}

static void jaeger_sampler_noop_close(jaeger_sampler* sampler)
{
    (void) sampler;
}

void jaeger_const_sampler_init(jaeger_const_sampler* sampler, bool decision)
{
    assert(sampler != NULL);
    sampler->is_sampled = &jaeger_const_sampler_is_sampled;
    sampler->close = &jaeger_sampler_noop_close;
}

static bool
jaeger_probabilistic_sampler_is_sampled(jaeger_sampler* sampler,
                                        const jaeger_trace_id* trace_id,
                                        const char* operation_name,
                                        jaeger_tag_list* tags)
{
    (void) trace_id;
    (void) operation_name;
    jaeger_probabilistic_sampler* s = (jaeger_probabilistic_sampler*) sampler;
#ifdef HAVE_RAND_R
    const double threshold = ((double) rand_r(&s->seed)) / RAND_MAX;
#else
    const double threshold = ((double) rand()) / RAND_MAX;
#endif /* HAVE_RAND_R */
    const bool decision = (s->sampling_rate >= threshold);
    if (tags != NULL) {
        jaeger_tag tag = JAEGERTRACING__PROTOBUF__TAG__INIT;
        tag.key = JAEGERTRACINGC_SAMPLER_TYPE_TAG_KEY;
        tag.value_case = JAEGERTRACING__PROTOBUF__TAG__VALUE_STR_VALUE;
        tag.str_value = JAEGERTRACINGC_SAMPLER_TYPE_PROBABILISTIC;
        jaeger_tag_list_append(tags, &tag);

        tag.key = JAEGERTRACINGC_SAMPLER_PARAM_TAG_KEY;
        tag.value_case = JAEGERTRACING__PROTOBUF__TAG__VALUE_DOUBLE_VALUE;
        tag.double_value = s->sampling_rate;
        jaeger_tag_list_append(tags, &tag);
    }
    return decision;
}

void jaeger_probabilistic_sampler_init(jaeger_probabilistic_sampler* sampler,
                                       double sampling_rate)
{
    assert(sampler != NULL);
    sampler->is_sampled = &jaeger_probabilistic_sampler_is_sampled;
    sampler->close = &jaeger_sampler_noop_close;
    sampler->sampling_rate =
        (sampling_rate < 0) ? 0 : ((sampling_rate > 1) ? 1 : sampling_rate);
}

static bool
jaeger_rate_limiting_sampler_is_sampled(jaeger_sampler* sampler,
                                        const jaeger_trace_id* trace_id,
                                        const char* operation_name,
                                        jaeger_tag_list* tags)
{
    (void) trace_id;
    (void) operation_name;
    assert(sampler != NULL);
    jaeger_rate_limiting_sampler* s = (jaeger_rate_limiting_sampler*) sampler;
    const bool decision = jaeger_token_bucket_check_credit(&s->tok, 1);
    if (tags != NULL) {
        jaeger_tag tag = JAEGERTRACING__PROTOBUF__TAG__INIT;
        tag.key = JAEGERTRACINGC_SAMPLER_TYPE_TAG_KEY;
        tag.value_case = JAEGERTRACING__PROTOBUF__TAG__VALUE_STR_VALUE;
        tag.str_value = JAEGERTRACINGC_SAMPLER_TYPE_RATE_LIMITING;
        jaeger_tag_list_append(tags, &tag);

        tag.key = JAEGERTRACINGC_SAMPLER_PARAM_TAG_KEY;
        tag.value_case = JAEGERTRACING__PROTOBUF__TAG__VALUE_DOUBLE_VALUE;
        tag.double_value = s->max_traces_per_second;
        jaeger_tag_list_append(tags, &tag);
    }
    return decision;
}

void jaeger_rate_limiting_sampler_init(jaeger_rate_limiting_sampler* sampler,
                                       double max_traces_per_second)
{
    assert(sampler != NULL);
    sampler->is_sampled = &jaeger_rate_limiting_sampler_is_sampled;
    sampler->close = &jaeger_sampler_noop_close;
    jaeger_token_bucket_init(
        &sampler->tok,
        max_traces_per_second,
        (max_traces_per_second < 1) ? 1 : max_traces_per_second);
    sampler->max_traces_per_second = max_traces_per_second;
}

static bool jaeger_guaranteed_throughput_probabilistic_sampler_is_sampled(
    jaeger_sampler* sampler,
    const jaeger_trace_id* trace_id,
    const char* operation_name,
    jaeger_tag_list* tags)
{
    (void) trace_id;
    (void) operation_name;
    assert(sampler != NULL);
    jaeger_guaranteed_throughput_probabilistic_sampler* s =
        (jaeger_guaranteed_throughput_probabilistic_sampler*) sampler;
    bool decision = s->probabilistic_sampler.is_sampled(
        (jaeger_sampler*) &s->probabilistic_sampler,
        trace_id,
        operation_name,
        NULL);
    if (decision) {
        s->lower_bound_sampler.is_sampled(
            (jaeger_sampler*) &s->lower_bound_sampler,
            trace_id,
            operation_name,
            NULL);
        if (tags != NULL) {
            jaeger_tag tag = JAEGERTRACING__PROTOBUF__TAG__INIT;
            tag.key = JAEGERTRACINGC_SAMPLER_TYPE_TAG_KEY;
            tag.value_case = JAEGERTRACING__PROTOBUF__TAG__VALUE_STR_VALUE;
            tag.str_value = JAEGERTRACINGC_SAMPLER_TYPE_PROBABILISTIC;
            jaeger_tag_list_append(tags, &tag);

            tag.key = JAEGERTRACINGC_SAMPLER_PARAM_TAG_KEY;
            tag.value_case = JAEGERTRACING__PROTOBUF__TAG__VALUE_DOUBLE_VALUE;
            tag.double_value = s->probabilistic_sampler.sampling_rate;
            jaeger_tag_list_append(tags, &tag);
        }
        return true;
    }
    decision = s->lower_bound_sampler.is_sampled(
        (jaeger_sampler*) &s->lower_bound_sampler,
        trace_id,
        operation_name,
        NULL);
    if (tags != NULL) {
        jaeger_tag tag = JAEGERTRACING__PROTOBUF__TAG__INIT;
        tag.key = JAEGERTRACINGC_SAMPLER_TYPE_TAG_KEY;
        tag.value_case = JAEGERTRACING__PROTOBUF__TAG__VALUE_STR_VALUE;
        tag.str_value = JAEGERTRACINGC_SAMPLER_TYPE_RATE_LIMITING;
        jaeger_tag_list_append(tags, &tag);

        tag.key = JAEGERTRACINGC_SAMPLER_PARAM_TAG_KEY;
        tag.value_case = JAEGERTRACING__PROTOBUF__TAG__VALUE_DOUBLE_VALUE;
        tag.double_value = s->lower_bound_sampler.max_traces_per_second;
        jaeger_tag_list_append(tags, &tag);
    }
    return decision;
}

static void jaeger_guaranteed_throughput_probabilistic_sampler_close(
    jaeger_sampler* sampler)
{
    assert(sampler != NULL);
    jaeger_guaranteed_throughput_probabilistic_sampler* s =
        (jaeger_guaranteed_throughput_probabilistic_sampler*) sampler;
    s->probabilistic_sampler.close((jaeger_sampler*) &s->probabilistic_sampler);
    s->lower_bound_sampler.close((jaeger_sampler*) &s->lower_bound_sampler);
}

void jaeger_guaranteed_throughput_probabilistic_sampler_init(
    jaeger_guaranteed_throughput_probabilistic_sampler* sampler,
    double lower_bound,
    double sampling_rate)
{
    assert(sampler != NULL);
    sampler->is_sampled =
        &jaeger_guaranteed_throughput_probabilistic_sampler_is_sampled;
    sampler->close = &jaeger_guaranteed_throughput_probabilistic_sampler_close;
    jaeger_probabilistic_sampler_init(&sampler->probabilistic_sampler,
                                      sampling_rate);
    jaeger_rate_limiting_sampler_init(&sampler->lower_bound_sampler,
                                      lower_bound);
}

void jaeger_guaranteed_throughput_probabilistic_sampler_update(
    jaeger_guaranteed_throughput_probabilistic_sampler* sampler,
    double lower_bound,
    double sampling_rate)
{
    assert(sampler != NULL);
    if (sampler->probabilistic_sampler.sampling_rate != sampling_rate) {
        sampler->probabilistic_sampler.close(
            (jaeger_sampler*) &sampler->probabilistic_sampler);
        jaeger_probabilistic_sampler_init(&sampler->probabilistic_sampler,
                                          sampling_rate);
    }
    if (sampler->lower_bound_sampler.max_traces_per_second != lower_bound) {
        sampler->lower_bound_sampler.close(
            (jaeger_sampler*) &sampler->lower_bound_sampler);
        jaeger_rate_limiting_sampler_init(&sampler->lower_bound_sampler,
                                          lower_bound);
    }
}

typedef Jaegertracing__Protobuf__SamplingManager__PerOperationSamplingStrategy__OperationSamplingStrategy
    jaeger_operation_sampling_strategy;

static inline jaeger_operation_sampler* samplers_from_strategies(
    const jaeger_per_operation_sampling_strategy* strategies,
    int* const num_op_samplers)
{
    assert(strategies != NULL);
    assert(num_op_samplers != NULL);
    *num_op_samplers = 0;
    for (int i = 0; i < strategies->n_per_operation_strategy; i++) {
        const jaeger_operation_sampling_strategy* strategy =
            strategies->per_operation_strategy[i];
        if (strategy == NULL) {
            fprintf(stderr, "WARNING: Encountered null operation strategy\n");
            continue;
        }
        if (strategy->probabilistic == NULL) {
            fprintf(stderr,
                    "WARNING: Encountered null probabilistic strategy\n");
            continue;
        }
        (*num_op_samplers)++;
    }

    const size_t op_samplers_size =
        sizeof(jaeger_operation_sampler) * (*num_op_samplers);
    jaeger_operation_sampler* op_samplers = jaeger_malloc(op_samplers_size);
    if (op_samplers == NULL) {
        fprintf(stderr, "ERROR: Cannot allocate per operation samplers\n");
        return NULL;
    }
    memset(op_samplers, 0, op_samplers_size);

    bool success = true;
    int index = 0;
    for (int i = 0; i < strategies->n_per_operation_strategy; i++) {
        const jaeger_operation_sampling_strategy* strategy =
            strategies->per_operation_strategy[i];
        if (strategy == NULL || strategy->probabilistic == NULL) {
            continue;
        }
        jaeger_operation_sampler* op_sampler = &op_samplers[index];
        op_sampler->operation_name = jaeger_strdup(strategy->operation);
        if (op_sampler->operation_name == NULL) {
            fprintf(stderr,
                    "ERROR: Cannot allocate operation sampler, "
                    "operation name = \"%s\"\n",
                    strategy->operation);
            success = false;
            break;
        }

        jaeger_guaranteed_throughput_probabilistic_sampler_init(
            &op_sampler->sampler,
            strategies->default_lower_bound_traces_per_second,
            strategy->probabilistic->sampling_rate);
        index++;
    }

    if (!success) {
        for (int i = 0; i < index; i++) {
            jaeger_operation_sampler_destroy(&op_samplers[i]);
        }
        jaeger_free(op_samplers);
        return NULL;
    }

    return op_samplers;
}

static inline bool
jaeger_adaptive_sampler_resize_op_samplers(jaeger_adaptive_sampler* sampler)
{
    assert(sampler != NULL);
    assert(sampler->num_op_samplers <= sampler->op_samplers_capacity);
    const int new_capacity =
        JAEGERTRACINGC_MIN(sampler->num_op_samplers * SAMPLER_GROWTH_RATIO,
                           sampler->max_operations);
    jaeger_operation_sampler* new_op_samplers = jaeger_realloc(
        sampler->op_samplers, sizeof(jaeger_operation_sampler) * new_capacity);
    if (new_op_samplers == NULL) {
        return false;
    }

    sampler->op_samplers = new_op_samplers;
    sampler->op_samplers_capacity = new_capacity;
    return true;
}

static bool jaeger_adaptive_sampler_is_sampled(jaeger_sampler* sampler,
                                               const jaeger_trace_id* trace_id,
                                               const char* operation_name,
                                               jaeger_tag_list* tags)
{
    (void) trace_id;
    (void) operation_name;
    assert(sampler != NULL);
    jaeger_adaptive_sampler* s = (jaeger_adaptive_sampler*) sampler;
    jaeger_mutex_lock(&s->mutex);
    for (int i = 0; i < s->num_op_samplers; i++) {
        if (strcmp(s->op_samplers[i].operation_name, operation_name) == 0) {
            jaeger_guaranteed_throughput_probabilistic_sampler* g =
                &s->op_samplers[i].sampler;
            const bool decision = g->is_sampled(
                (jaeger_sampler*) g, trace_id, operation_name, tags);
            jaeger_mutex_unlock(&s->mutex);
            return decision;
        }
    }

    if (s->num_op_samplers >= s->max_operations) {
        goto default_sampler;
    }

    if (s->num_op_samplers == s->op_samplers_capacity &&
        !jaeger_adaptive_sampler_resize_op_samplers(s)) {
        fprintf(stderr,
                "ERROR: Cannot allocate more operation samplers "
                "in adaptive sampler\n");
        goto default_sampler;
    }
    jaeger_operation_sampler* op_sampler = &s->op_samplers[s->num_op_samplers];
    op_sampler->operation_name = jaeger_strdup(operation_name);
    if (op_sampler->operation_name == NULL) {
        fprintf(stderr,
                "ERROR: Cannot allocate operation name for new "
                "operation sampler in adaptive sampler\n");
        goto default_sampler;
    }
    jaeger_guaranteed_throughput_probabilistic_sampler_init(
        &op_sampler->sampler, s->lower_bound, s->default_sampler.sampling_rate);
    s->num_op_samplers++;

    const bool decision = op_sampler->sampler.is_sampled(
        (jaeger_sampler*) &op_sampler->sampler, trace_id, operation_name, tags);
    jaeger_mutex_unlock(&s->mutex);
    return decision;

default_sampler : {
    const bool decision = s->default_sampler.is_sampled(
        (jaeger_sampler*) &s->default_sampler, trace_id, operation_name, tags);
    jaeger_mutex_unlock(&s->mutex);
    return decision;
}
}

static void jaeger_adaptive_sampler_close(jaeger_sampler* sampler)
{
    assert(sampler != NULL);
    jaeger_adaptive_sampler* s = (jaeger_adaptive_sampler*) sampler;
    for (int i = 0; i < s->num_op_samplers; i++) {
        jaeger_operation_sampler_destroy(&s->op_samplers[i]);
    }
    jaeger_free(s->op_samplers);
    jaeger_mutex_destroy(&s->mutex);
}

bool jaeger_adaptive_sampler_init(
    jaeger_adaptive_sampler* sampler,
    const jaeger_per_operation_sampling_strategy* strategies,
    int max_operations)
{
    assert(sampler != NULL);
    sampler->op_samplers =
        samplers_from_strategies(strategies, &sampler->num_op_samplers);
    if (sampler->op_samplers == NULL) {
        return false;
    }
    sampler->op_samplers_capacity = sampler->num_op_samplers;
    sampler->max_operations = max_operations;
    sampler->mutex = (jaeger_mutex) JAEGER_MUTEX_INIT;
    jaeger_mutex_init(&sampler->mutex, NULL);
    sampler->is_sampled = &jaeger_adaptive_sampler_is_sampled;
    sampler->close = &jaeger_adaptive_sampler_close;
    return true;
}

void jaeger_adaptive_sampler_update(
    jaeger_adaptive_sampler* sampler,
    const jaeger_per_operation_sampling_strategy* strategies)
{
    assert(sampler != NULL);
    assert(strategies != NULL);
    const double lower_bound =
        strategies->default_lower_bound_traces_per_second;
    jaeger_mutex_lock(&sampler->mutex);
    for (int i = 0; i < strategies->n_per_operation_strategy; i++) {
        const jaeger_operation_sampling_strategy* strategy =
            strategies->per_operation_strategy[i];
        bool found_match = false;
        if (strategy == NULL) {
            fprintf(stderr, "WARNING: Encountered null operation strategy\n");
        }
        if (strategy->probabilistic == NULL) {
            fprintf(stderr,
                    "WARNING: Encountered null probabilistic strategy\n");
            continue;
        }

        for (int j = 0; j < sampler->num_op_samplers; j++) {
            jaeger_operation_sampler* op_sampler = &sampler->op_samplers[j];
            if (strcmp(op_sampler->operation_name, strategy->operation) == 0) {
                jaeger_guaranteed_throughput_probabilistic_sampler_update(
                    &op_sampler->sampler,
                    lower_bound,
                    strategy->probabilistic->sampling_rate);
                found_match = true;
                break;
            }
        }

        if (!found_match) {
            if (sampler->num_op_samplers == sampler->op_samplers_capacity &&
                !jaeger_adaptive_sampler_resize_op_samplers(sampler)) {
                fprintf(stderr,
                        "ERROR: Cannot allocate more operation samplers "
                        "in adaptive sampler\n");
                continue;
            }
            jaeger_operation_sampler* op_sampler =
                &sampler->op_samplers[sampler->num_op_samplers];
            op_sampler->operation_name = jaeger_strdup(strategy->operation);
            if (op_sampler->operation_name == NULL) {
                fprintf(stderr,
                        "ERROR: Cannot allocate operation sampler, "
                        "operation name = \"%s\"\n",
                        strategy->operation);
                continue;
            }
            sampler->num_op_samplers++;
        }
    }
    jaeger_mutex_unlock(&sampler->mutex);
}

void jaeger_remotely_controlled_sampler_init(
    jaeger_remotely_controlled_sampler* sampler,
    char* service_name,
    jaeger_sampler* initial_sampler,
    int max_operations,
    const jaeger_duration* sampling_refresh_interval,
    jaeger_metrics* metrics)
{
    assert(sampler != NULL);
    /* TODO */
}
