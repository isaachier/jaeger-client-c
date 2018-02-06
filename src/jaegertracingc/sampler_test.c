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
#include <string.h>
#include "unity.h"

#define SET_UP_SAMPLER_TEST()                                                 \
    jaeger_tag_list tags;                                                     \
    jaeger_tag_list_init(&tags);                                              \
    const char* operation_name = "test-operation";                            \
    (void) operation_name;                                                    \
    const jaeger_trace_id trace_id = JAEGERTRACING__PROTOBUF__TRACE_ID__INIT; \
    (void) trace_id

#define TEAR_DOWN_SAMPLER_TEST(sampler)        \
    sampler.close((jaeger_sampler*) &sampler); \
    jaeger_tag_list_destroy(&tags)

#define TEST_DEFAULT_SAMPLING_PROBABILITY 0.5

void test_const_sampler()
{
    SET_UP_SAMPLER_TEST();

    jaeger_const_sampler c;
    jaeger_const_sampler_init(&c, true);

    TEST_ASSERT_TRUE(
        c.is_sampled((jaeger_sampler*) &c, &trace_id, operation_name, &tags));
    c.close((jaeger_sampler*) &c);
    TEST_ASSERT_EQUAL(2, tags.size);
    TEST_ASSERT_EQUAL_STRING(JAEGERTRACINGC_SAMPLER_TYPE_TAG_KEY,
                             tags.tags[0].key);
    TEST_ASSERT_EQUAL(JAEGERTRACING__PROTOBUF__TAG__VALUE_STR_VALUE,
                      tags.tags[0].value_case);
    TEST_ASSERT_EQUAL_STRING(JAEGERTRACINGC_SAMPLER_TYPE_CONST,
                             tags.tags[0].str_value);
    TEST_ASSERT_EQUAL_STRING(JAEGERTRACINGC_SAMPLER_PARAM_TAG_KEY,
                             tags.tags[1].key);
    TEST_ASSERT_EQUAL(JAEGERTRACING__PROTOBUF__TAG__VALUE_BOOL_VALUE,
                      tags.tags[1].value_case);
    TEST_ASSERT_TRUE(tags.tags[1].bool_value);

    jaeger_tag_list_clear(&tags);
    jaeger_const_sampler_init(&c, false);
    TEST_ASSERT_FALSE(
        c.is_sampled((jaeger_sampler*) &c, &trace_id, operation_name, &tags));
    TEST_ASSERT_EQUAL(2, tags.size);
    TEST_ASSERT_EQUAL_STRING(JAEGERTRACINGC_SAMPLER_TYPE_TAG_KEY,
                             tags.tags[0].key);
    TEST_ASSERT_EQUAL(JAEGERTRACING__PROTOBUF__TAG__VALUE_STR_VALUE,
                      tags.tags[0].value_case);
    TEST_ASSERT_EQUAL_STRING(JAEGERTRACINGC_SAMPLER_TYPE_CONST,
                             tags.tags[0].str_value);
    TEST_ASSERT_EQUAL_STRING(JAEGERTRACINGC_SAMPLER_PARAM_TAG_KEY,
                             tags.tags[1].key);
    TEST_ASSERT_EQUAL(JAEGERTRACING__PROTOBUF__TAG__VALUE_BOOL_VALUE,
                      tags.tags[1].value_case);
    TEST_ASSERT_FALSE(tags.tags[1].bool_value);

    TEAR_DOWN_SAMPLER_TEST(c);
}

void test_probabilistic_sampler()
{
    SET_UP_SAMPLER_TEST();

    jaeger_probabilistic_sampler p;
    jaeger_probabilistic_sampler_init(&p, 1);
    TEST_ASSERT_TRUE(
        p.is_sampled((jaeger_sampler*) &p, &trace_id, operation_name, &tags));
    p.close((jaeger_sampler*) &p);

    TEST_ASSERT_EQUAL(2, tags.size);

    TEST_ASSERT_EQUAL_STRING(JAEGERTRACINGC_SAMPLER_TYPE_TAG_KEY,
                             tags.tags[0].key);
    TEST_ASSERT_EQUAL(JAEGERTRACING__PROTOBUF__TAG__VALUE_STR_VALUE,
                      tags.tags[0].value_case);
    TEST_ASSERT_EQUAL_STRING(JAEGERTRACINGC_SAMPLER_TYPE_PROBABILISTIC,
                             tags.tags[0].str_value);

    TEST_ASSERT_EQUAL_STRING(JAEGERTRACINGC_SAMPLER_PARAM_TAG_KEY,
                             tags.tags[1].key);
    TEST_ASSERT_EQUAL(JAEGERTRACING__PROTOBUF__TAG__VALUE_DOUBLE_VALUE,
                      tags.tags[1].value_case);
    TEST_ASSERT_EQUAL(1, tags.tags[1].double_value);
    jaeger_tag_list_clear(&tags);

    jaeger_probabilistic_sampler_init(&p, 0);
    TEST_ASSERT_FALSE(
        p.is_sampled((jaeger_sampler*) &p, &trace_id, operation_name, &tags));

    TEST_ASSERT_EQUAL(2, tags.size);

    TEST_ASSERT_EQUAL_STRING(JAEGERTRACINGC_SAMPLER_TYPE_TAG_KEY,
                             tags.tags[0].key);
    TEST_ASSERT_EQUAL(JAEGERTRACING__PROTOBUF__TAG__VALUE_STR_VALUE,
                      tags.tags[0].value_case);
    TEST_ASSERT_EQUAL_STRING(JAEGERTRACINGC_SAMPLER_TYPE_PROBABILISTIC,
                             tags.tags[0].str_value);

    TEST_ASSERT_EQUAL_STRING(JAEGERTRACINGC_SAMPLER_PARAM_TAG_KEY,
                             tags.tags[1].key);
    TEST_ASSERT_EQUAL(JAEGERTRACING__PROTOBUF__TAG__VALUE_DOUBLE_VALUE,
                      tags.tags[1].value_case);
    TEST_ASSERT_EQUAL(0, tags.tags[1].double_value);

    TEAR_DOWN_SAMPLER_TEST(p);
}

void test_rate_limiting_sampler()
{
    SET_UP_SAMPLER_TEST();

    const double max_traces_per_second = 1;
    jaeger_rate_limiting_sampler r;
    jaeger_rate_limiting_sampler_init(&r, max_traces_per_second);

    TEST_ASSERT_TRUE(
        r.is_sampled((jaeger_sampler*) &r, &trace_id, operation_name, &tags));
    TEST_ASSERT_EQUAL(2, tags.size);
    TEST_ASSERT_EQUAL_STRING(JAEGERTRACINGC_SAMPLER_TYPE_TAG_KEY,
                             tags.tags[0].key);
    TEST_ASSERT_EQUAL(JAEGERTRACING__PROTOBUF__TAG__VALUE_STR_VALUE,
                      tags.tags[0].value_case);
    TEST_ASSERT_EQUAL_STRING(JAEGERTRACINGC_SAMPLER_TYPE_RATE_LIMITING,
                             tags.tags[0].str_value);
    TEST_ASSERT_EQUAL_STRING(JAEGERTRACINGC_SAMPLER_PARAM_TAG_KEY,
                             tags.tags[1].key);
    TEST_ASSERT_EQUAL(JAEGERTRACING__PROTOBUF__TAG__VALUE_DOUBLE_VALUE,
                      tags.tags[1].value_case);
    TEST_ASSERT_EQUAL(max_traces_per_second, tags.tags[1].double_value);

    jaeger_tag_list_clear(&tags);
    TEST_ASSERT_FALSE(
        r.is_sampled((jaeger_sampler*) &r, &trace_id, operation_name, &tags));
    TEST_ASSERT_EQUAL_STRING(JAEGERTRACINGC_SAMPLER_TYPE_TAG_KEY,
                             tags.tags[0].key);
    TEST_ASSERT_EQUAL(JAEGERTRACING__PROTOBUF__TAG__VALUE_STR_VALUE,
                      tags.tags[0].value_case);
    TEST_ASSERT_EQUAL_STRING(JAEGERTRACINGC_SAMPLER_TYPE_RATE_LIMITING,
                             tags.tags[0].str_value);
    TEST_ASSERT_EQUAL_STRING(JAEGERTRACINGC_SAMPLER_PARAM_TAG_KEY,
                             tags.tags[1].key);
    TEST_ASSERT_EQUAL(JAEGERTRACING__PROTOBUF__TAG__VALUE_DOUBLE_VALUE,
                      tags.tags[1].value_case);
    TEST_ASSERT_EQUAL(max_traces_per_second, tags.tags[1].double_value);

    TEAR_DOWN_SAMPLER_TEST(r);
}

void test_guaranteed_throughput_probabilistic_sampler()
{
    SET_UP_SAMPLER_TEST();

    double sampling_rate = TEST_DEFAULT_SAMPLING_PROBABILITY;
    double lower_bound = 2.0;
    jaeger_guaranteed_throughput_probabilistic_sampler g;
    jaeger_guaranteed_throughput_probabilistic_sampler_init(
        &g, lower_bound, sampling_rate);
    TEST_ASSERT_EQUAL(sampling_rate, g.probabilistic_sampler.sampling_rate);
    TEST_ASSERT_EQUAL(lower_bound, g.lower_bound_sampler.max_traces_per_second);

    sampling_rate = 0.6;
    lower_bound = 1.0;
    jaeger_guaranteed_throughput_probabilistic_sampler_update(
        &g, lower_bound, sampling_rate);
    TEST_ASSERT_EQUAL(sampling_rate, g.probabilistic_sampler.sampling_rate);
    TEST_ASSERT_EQUAL(lower_bound, g.lower_bound_sampler.max_traces_per_second);

    sampling_rate = 1.1;
    lower_bound = 1.0;
    jaeger_guaranteed_throughput_probabilistic_sampler_update(
        &g, lower_bound, sampling_rate);
    TEST_ASSERT_EQUAL(1.0, g.probabilistic_sampler.sampling_rate);
    TEST_ASSERT_EQUAL(lower_bound, g.lower_bound_sampler.max_traces_per_second);

    sampling_rate = 0.0;
    jaeger_guaranteed_throughput_probabilistic_sampler_update(
        &g, lower_bound, sampling_rate);
    TEST_ASSERT_TRUE(
        g.is_sampled((jaeger_sampler*) &g, &trace_id, operation_name, &tags));
    TEST_ASSERT_EQUAL(2, tags.size);
    TEST_ASSERT_EQUAL_STRING(JAEGERTRACINGC_SAMPLER_TYPE_TAG_KEY,
                             tags.tags[0].key);
    TEST_ASSERT_EQUAL(JAEGERTRACING__PROTOBUF__TAG__VALUE_STR_VALUE,
                      tags.tags[0].value_case);
    TEST_ASSERT_EQUAL_STRING(JAEGERTRACINGC_SAMPLER_TYPE_LOWER_BOUND,
                             tags.tags[0].str_value);
    TEST_ASSERT_EQUAL_STRING(JAEGERTRACINGC_SAMPLER_PARAM_TAG_KEY,
                             tags.tags[1].key);
    TEST_ASSERT_EQUAL(JAEGERTRACING__PROTOBUF__TAG__VALUE_DOUBLE_VALUE,
                      tags.tags[1].value_case);
    TEST_ASSERT_EQUAL(lower_bound, tags.tags[1].double_value);

    jaeger_tag_list_clear(&tags);
    sampling_rate = 1.0;
    jaeger_guaranteed_throughput_probabilistic_sampler_update(
        &g, lower_bound, sampling_rate);
    TEST_ASSERT_TRUE(
        g.is_sampled((jaeger_sampler*) &g, &trace_id, operation_name, &tags));
    TEST_ASSERT_EQUAL(2, tags.size);
    TEST_ASSERT_EQUAL_STRING(JAEGERTRACINGC_SAMPLER_TYPE_TAG_KEY,
                             tags.tags[0].key);
    TEST_ASSERT_EQUAL(JAEGERTRACING__PROTOBUF__TAG__VALUE_STR_VALUE,
                      tags.tags[0].value_case);
    TEST_ASSERT_EQUAL_STRING(JAEGERTRACINGC_SAMPLER_TYPE_PROBABILISTIC,
                             tags.tags[0].str_value);
    TEST_ASSERT_EQUAL_STRING(JAEGERTRACINGC_SAMPLER_PARAM_TAG_KEY,
                             tags.tags[1].key);
    TEST_ASSERT_EQUAL(JAEGERTRACING__PROTOBUF__TAG__VALUE_DOUBLE_VALUE,
                      tags.tags[1].value_case);
    TEST_ASSERT_EQUAL(sampling_rate, tags.tags[1].double_value);

    TEAR_DOWN_SAMPLER_TEST(g);
}

#define TEST_DEFAULT_MAX_OPERATIONS 10

void test_adaptive_sampler()
{
    SET_UP_SAMPLER_TEST();

    jaeger_adaptive_sampler a;
    jaeger_per_operation_strategy strategies =
        JAEGERTRACING__PROTOBUF__SAMPLING_MANAGER__PER_OPERATION_SAMPLING_STRATEGY__INIT;
    strategies.per_operation_strategy =
        jaeger_malloc(sizeof(jaeger_operation_strategy*));
    TEST_ASSERT_NOT_NULL(strategies.per_operation_strategy);
    strategies.n_per_operation_strategy = 1;
    strategies.per_operation_strategy[0] =
        jaeger_malloc(sizeof(jaeger_operation_sampler));
    TEST_ASSERT_NOT_NULL(strategies.per_operation_strategy[0]);
    *strategies.per_operation_strategy[0] = (jaeger_operation_strategy)
        JAEGERTRACING__PROTOBUF__SAMPLING_MANAGER__PER_OPERATION_SAMPLING_STRATEGY__OPERATION_SAMPLING_STRATEGY__INIT;
    strategies.per_operation_strategy[0]->operation =
        jaeger_strdup(operation_name);
    TEST_ASSERT_NOT_NULL(strategies.per_operation_strategy[0]->operation);
    strategies.per_operation_strategy[0]->strategy_case =
        JAEGERTRACING__PROTOBUF__SAMPLING_MANAGER__PER_OPERATION_SAMPLING_STRATEGY__OPERATION_SAMPLING_STRATEGY__STRATEGY_PROBABILISTIC;
    strategies.per_operation_strategy[0]->probabilistic = jaeger_malloc(sizeof(
        Jaegertracing__Protobuf__SamplingManager__ProbabilisticSamplingStrategy));
    TEST_ASSERT_NOT_NULL(strategies.per_operation_strategy[0]->probabilistic);
    *strategies.per_operation_strategy[0]->probabilistic =
        (Jaegertracing__Protobuf__SamplingManager__ProbabilisticSamplingStrategy)
            JAEGERTRACING__PROTOBUF__SAMPLING_MANAGER__PROBABILISTIC_SAMPLING_STRATEGY__INIT;
    strategies.per_operation_strategy[0]->probabilistic->sampling_rate =
        TEST_DEFAULT_SAMPLING_PROBABILITY;
    strategies.default_lower_bound_traces_per_second = 1.0;
    strategies.default_sampling_probability = TEST_DEFAULT_SAMPLING_PROBABILITY;

    jaeger_adaptive_sampler_init(&a, &strategies, TEST_DEFAULT_MAX_OPERATIONS);

    jaeger_per_operation_strategy_destroy(&strategies);
    TEAR_DOWN_SAMPLER_TEST(a);
}

void test_remotely_controlled_sampler()
{
    /* TODO */
}
