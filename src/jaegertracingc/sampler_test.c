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

#include "jaegertracingc/mock_agent.h"
#include "jaegertracingc/sampler.h"
#include "jaegertracingc/threading.h"
#include "unity.h"

#define SET_UP_SAMPLER_TEST()                                      \
    jaeger_vector tags;                                            \
    jaeger_vector_init(&tags, sizeof(jaeger_tag));                 \
    const char* operation_name = "test-operation";                 \
    (void) operation_name;                                         \
    const jaeger_trace_id trace_id = JAEGERTRACINGC_TRACE_ID_INIT; \
    (void) trace_id

#define TEAR_DOWN_SAMPLER_TEST(sampler)                                    \
    ((jaeger_destructible*) &sampler)                                      \
        ->destroy((jaeger_destructible*) &sampler);                        \
    JAEGERTRACINGC_VECTOR_FOR_EACH(&tags, jaeger_tag_destroy, jaeger_tag); \
    jaeger_vector_destroy(&tags)

#define TEST_DEFAULT_SAMPLING_PROBABILITY 0.5
#define TEST_DEFAULT_MAX_TRACES_PER_SECOND 3
#define TEST_DEFAULT_MAX_OPERATIONS 10

#define CHECK_TAGS(                                                          \
    sampler_type, param_type, value_member, param_value, tag_list)           \
    do {                                                                     \
        TEST_ASSERT_EQUAL(2, jaeger_vector_length(&tag_list));               \
        jaeger_tag* tag = jaeger_vector_get(&tag_list, 0);                   \
        TEST_ASSERT_NOT_NULL(tag);                                           \
        TEST_ASSERT_EQUAL_STRING(JAEGERTRACINGC_SAMPLER_TYPE_TAG_KEY,        \
                                 tag->key);                                  \
        TEST_ASSERT_EQUAL(JAEGERTRACINGC_TAG_TYPE(STR), tag->value_case);    \
        TEST_ASSERT_EQUAL_STRING(JAEGERTRACINGC_SAMPLER_TYPE_##sampler_type, \
                                 tag->str_value);                            \
        tag = jaeger_vector_get(&tag_list, 1);                               \
        TEST_ASSERT_NOT_NULL(tag);                                           \
        TEST_ASSERT_EQUAL_STRING(JAEGERTRACINGC_SAMPLER_PARAM_TAG_KEY,       \
                                 tag->key);                                  \
        TEST_ASSERT_EQUAL(JAEGERTRACINGC_TAG_TYPE(param_type),               \
                          tag->value_case);                                  \
        TEST_ASSERT_EQUAL((param_value), tag->value_member);                 \
    } while (0)

#define CHECK_CONST_TAGS(sampler, tags) \
    CHECK_TAGS(CONST, BOOL, bool_value, (sampler).decision, (tags))

#define CHECK_PROBABILISTIC_TAGS(sampler, tags) \
    CHECK_TAGS(                                 \
        PROBABILISTIC, DOUBLE, double_value, (sampler).sampling_rate, (tags))

#define CHECK_RATE_LIMITING_TAGS(sampler, tags) \
    CHECK_TAGS(RATE_LIMITING,                   \
               DOUBLE,                          \
               double_value,                    \
               (sampler).max_traces_per_second, \
               (tags))

#define CHECK_LOWER_BOUND_TAGS(sampler, tags)                       \
    CHECK_TAGS(LOWER_BOUND,                                         \
               DOUBLE,                                              \
               double_value,                                        \
               (sampler).lower_bound_sampler.max_traces_per_second, \
               (tags))

static inline void test_const_sampler()
{
    SET_UP_SAMPLER_TEST();

    jaeger_const_sampler c;
    jaeger_const_sampler_init(&c, true);

    TEST_ASSERT_TRUE(
        ((jaeger_sampler*) &c)
            ->is_sampled(
                (jaeger_sampler*) &c, &trace_id, operation_name, &tags));
    CHECK_CONST_TAGS(c, tags);

    ((jaeger_destructible*) &c)->destroy((jaeger_destructible*) &c);
    JAEGERTRACINGC_VECTOR_FOR_EACH(&tags, jaeger_tag_destroy, jaeger_tag);
    jaeger_vector_clear(&tags);
    jaeger_const_sampler_init(&c, false);
    TEST_ASSERT_FALSE(
        ((jaeger_sampler*) &c)
            ->is_sampled(
                (jaeger_sampler*) &c, &trace_id, operation_name, &tags));

    TEAR_DOWN_SAMPLER_TEST(c);
}

static inline void test_probabilistic_sampler()
{
    SET_UP_SAMPLER_TEST();

    jaeger_probabilistic_sampler p;
    double sampling_rate = 1;
    jaeger_probabilistic_sampler_init(&p, sampling_rate);
    TEST_ASSERT_TRUE(
        ((jaeger_sampler*) &p)
            ->is_sampled(
                (jaeger_sampler*) &p, &trace_id, operation_name, &tags));
    CHECK_PROBABILISTIC_TAGS(p, tags);
    ((jaeger_destructible*) &p)->destroy((jaeger_destructible*) &p);

    sampling_rate = 0;
    JAEGERTRACINGC_VECTOR_FOR_EACH(&tags, jaeger_tag_destroy, jaeger_tag);
    jaeger_vector_clear(&tags);
    jaeger_probabilistic_sampler_init(&p, sampling_rate);
    TEST_ASSERT_FALSE(
        ((jaeger_sampler*) &p)
            ->is_sampled(
                (jaeger_sampler*) &p, &trace_id, operation_name, &tags));
    CHECK_PROBABILISTIC_TAGS(p, tags);

    TEAR_DOWN_SAMPLER_TEST(p);
}

static inline void test_rate_limiting_sampler()
{
    SET_UP_SAMPLER_TEST();

    const double max_traces_per_second = 1;
    jaeger_rate_limiting_sampler r;
    jaeger_rate_limiting_sampler_init(&r, max_traces_per_second);

    TEST_ASSERT_TRUE(
        ((jaeger_sampler*) &r)
            ->is_sampled(
                (jaeger_sampler*) &r, &trace_id, operation_name, &tags));
    CHECK_RATE_LIMITING_TAGS(r, tags);

    JAEGERTRACINGC_VECTOR_FOR_EACH(&tags, jaeger_tag_destroy, jaeger_tag);
    jaeger_vector_clear(&tags);
    TEST_ASSERT_FALSE(
        ((jaeger_sampler*) &r)
            ->is_sampled(
                (jaeger_sampler*) &r, &trace_id, operation_name, &tags));
    CHECK_RATE_LIMITING_TAGS(r, tags);

    TEAR_DOWN_SAMPLER_TEST(r);
}

static inline void test_guaranteed_throughput_probabilistic_sampler()
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
        ((jaeger_sampler*) &g)
            ->is_sampled(
                (jaeger_sampler*) &g, &trace_id, operation_name, &tags));
    CHECK_LOWER_BOUND_TAGS(g, tags);

    JAEGERTRACINGC_VECTOR_FOR_EACH(&tags, jaeger_tag_destroy, jaeger_tag);
    jaeger_vector_clear(&tags);
    sampling_rate = 1.0;
    jaeger_guaranteed_throughput_probabilistic_sampler_update(
        &g, lower_bound, sampling_rate);
    TEST_ASSERT_TRUE(
        ((jaeger_sampler*) &g)
            ->is_sampled(
                (jaeger_sampler*) &g, &trace_id, operation_name, &tags));
    CHECK_PROBABILISTIC_TAGS(g.probabilistic_sampler, tags);

    TEAR_DOWN_SAMPLER_TEST(g);
}

static inline void test_adaptive_sampler()
{
    SET_UP_SAMPLER_TEST();

    jaeger_adaptive_sampler a;
    jaeger_per_operation_strategy strategies =
        JAEGERTRACINGC_PER_OPERATION_STRATEGY_INIT;
    strategies.per_operation_strategy =
        jaeger_malloc(sizeof(jaeger_operation_strategy*));
    TEST_ASSERT_NOT_NULL(strategies.per_operation_strategy);
    strategies.n_per_operation_strategy = 1;
    strategies.per_operation_strategy[0] =
        jaeger_malloc(sizeof(jaeger_operation_sampler));
    TEST_ASSERT_NOT_NULL(strategies.per_operation_strategy[0]);
    *strategies.per_operation_strategy[0] =
        (jaeger_operation_strategy) JAEGERTRACINGC_OPERATION_STRATEGY_INIT;
    strategies.per_operation_strategy[0]->operation =
        jaeger_strdup(operation_name);
    TEST_ASSERT_NOT_NULL(strategies.per_operation_strategy[0]->operation);
    strategies.per_operation_strategy[0]->strategy_case =
        JAEGERTRACINGC_OPERATION_STRATEGY_TYPE(PROBABILISTIC);
    strategies.per_operation_strategy[0]->probabilistic =
        jaeger_malloc(sizeof(jaeger_probabilistic_strategy));
    TEST_ASSERT_NOT_NULL(strategies.per_operation_strategy[0]->probabilistic);
    *strategies.per_operation_strategy[0]->probabilistic =
        (jaeger_probabilistic_strategy)
            JAEGERTRACINGC_PROBABILISTIC_STRATEGY_INIT;
    strategies.per_operation_strategy[0]->probabilistic->sampling_rate =
        TEST_DEFAULT_SAMPLING_PROBABILITY;
    strategies.default_lower_bound_traces_per_second = 1.0;
    strategies.default_sampling_probability = TEST_DEFAULT_SAMPLING_PROBABILITY;

    jaeger_adaptive_sampler_init(&a, &strategies, TEST_DEFAULT_MAX_OPERATIONS);
    ((jaeger_sampler*) &a)
        ->is_sampled((jaeger_sampler*) &a, &trace_id, operation_name, &tags);

    JAEGERTRACINGC_VECTOR_FOR_EACH(&tags, jaeger_tag_destroy, jaeger_tag);
    jaeger_vector_clear(&tags);
    for (int i = 0; i < TEST_DEFAULT_MAX_OPERATIONS; i++) {
        char op_buffer[strlen("new-operation") + 3];
        TEST_ASSERT_LESS_THAN(
            sizeof(op_buffer),
            snprintf(op_buffer, sizeof(op_buffer), "new-operation-%d", i));
        ((jaeger_sampler*) &a)
            ->is_sampled((jaeger_sampler*) &a, &trace_id, op_buffer, &tags);
    }
    TEST_ASSERT_EQUAL(TEST_DEFAULT_MAX_OPERATIONS,
                      jaeger_vector_length(&a.op_samplers));
    jaeger_operation_sampler* op_sampler_lhs =
        jaeger_vector_get(&a.op_samplers, 0);
    TEST_ASSERT_NOT_NULL(op_sampler_lhs);
    TEST_ASSERT_NOT_NULL(op_sampler_lhs->operation_name);
    jaeger_operation_sampler* op_sampler_rhs =
        jaeger_vector_get(&a.op_samplers, 1);
    TEST_ASSERT_NOT_NULL(op_sampler_rhs);
    TEST_ASSERT_NOT_NULL(op_sampler_rhs->operation_name);
    TEST_ASSERT_LESS_THAN(
        0,
        strcmp(op_sampler_lhs->operation_name, op_sampler_rhs->operation_name));

    jaeger_per_operation_strategy_destroy(&strategies);
    TEAR_DOWN_SAMPLER_TEST(a);
}

static inline void test_remotely_controlled_sampler()
{
    jaeger_metrics* metrics = jaeger_null_metrics();
    mock_http_server server = MOCK_HTTP_SERVER_INIT;
    mock_http_server_start(&server);
    const int port = ntohs(server.addr.sin_port);

#define URL_PREFIX "http://localhost:"
#define PORT_LEN (sizeof("65535") - 1)
    char buffer[sizeof(URL_PREFIX) + PORT_LEN];
    const int result = snprintf(buffer, sizeof(buffer), URL_PREFIX "%d", port);
    TEST_ASSERT_LESS_OR_EQUAL(sizeof(buffer) - 1, result);
    jaeger_remotely_controlled_sampler r;
    TEST_ASSERT_TRUE(
        jaeger_remotely_controlled_sampler_init(&r,
                                                "test-service",
                                                buffer,
                                                NULL,
                                                TEST_DEFAULT_MAX_OPERATIONS,
                                                metrics));

    const mock_http_response responses[] = {
        {.service_name = "test-service",
         .json_format = "{\n"
                        "  \"probabilisticSampling\": {\n"
                        "      \"samplingRate\": %f\n"
                        "    }\n"
                        "}\n",
         .arg_value = TEST_DEFAULT_SAMPLING_PROBABILITY},
        {.service_name = "test-service",
         .json_format = "{\n"
                        "  \"rateLimitingSampling\": {\n"
                        "      \"maxTracesPerSecond\": %0.0f\n"
                        "    }\n"
                        "}\n",
         .arg_value = TEST_DEFAULT_MAX_TRACES_PER_SECOND},
        {.service_name = "test-service",
         .json_format = "{\n"
                        "  \"operationSampling\": {\n"
                        "    \"defaultLowerBoundTracesPerSecond\": 1.0,\n"
                        "    \"defaultSamplingProbability\": 0.001,\n"
                        "    \"perOperationStrategies\": [{\n"
                        "      \"operation\": \"test-operation\",\n"
                        "      \"probabilisticSampling\": {\n"
                        "        \"samplingRate\": %f\n"
                        "      }\n"
                        "    }]\n"
                        "  }\n"
                        "}\n",
         .arg_value = TEST_DEFAULT_SAMPLING_PROBABILITY + 0.1},
        {.service_name = "test-service",
         .json_format = "{\n"
                        "  \"operationSampling\": {\n"
                        "    \"defaultLowerBoundTracesPerSecond\": 1.0,\n"
                        "    \"defaultSamplingProbability\": 0.001,\n"
                        "    \"perOperationStrategies\": [{\n"
                        "      \"operation\": \"test-operation\",\n"
                        "      \"probabilisticSampling\": {\n"
                        "        \"samplingRate\": %f\n"
                        "      }\n"
                        "    }]\n"
                        "  }\n"
                        "}\n",
         .arg_value = TEST_DEFAULT_SAMPLING_PROBABILITY - 0.1}};

    int index = 0;
    mock_http_server_set_response(&server, &responses[index]);
    index++;
    TEST_ASSERT_TRUE(jaeger_remotely_controlled_sampler_update(&r));
    TEST_ASSERT_EQUAL(jaeger_probabilistic_sampler_type, r.sampler.type);
    TEST_ASSERT_EQUAL(TEST_DEFAULT_SAMPLING_PROBABILITY,
                      r.sampler.probabilistic_sampler.sampling_rate);

    mock_http_server_set_response(&server, &responses[index]);
    index++;
    TEST_ASSERT_TRUE(jaeger_remotely_controlled_sampler_update(&r));
    TEST_ASSERT_EQUAL(jaeger_rate_limiting_sampler_type, r.sampler.type);
    TEST_ASSERT_EQUAL(TEST_DEFAULT_MAX_TRACES_PER_SECOND,
                      r.sampler.rate_limiting_sampler.max_traces_per_second);

    mock_http_server_set_response(&server, &responses[index]);
    index++;
    TEST_ASSERT_TRUE(jaeger_remotely_controlled_sampler_update(&r));
    TEST_ASSERT_EQUAL(jaeger_adaptive_sampler_type, r.sampler.type);
    TEST_ASSERT_EQUAL(
        1, jaeger_vector_length(&r.sampler.adaptive_sampler.op_samplers));
    jaeger_operation_sampler* op_sampler =
        jaeger_vector_get(&r.sampler.adaptive_sampler.op_samplers, 0);
    TEST_ASSERT_NOT_NULL(op_sampler);
    TEST_ASSERT_EQUAL_STRING("test-operation", op_sampler->operation_name);

    mock_http_server_set_response(&server, &responses[index]);
    index++;
    TEST_ASSERT_TRUE(jaeger_remotely_controlled_sampler_update(&r));
    TEST_ASSERT_EQUAL(jaeger_adaptive_sampler_type, r.sampler.type);
    TEST_ASSERT_EQUAL(
        1, jaeger_vector_length(&r.sampler.adaptive_sampler.op_samplers));
    op_sampler = jaeger_vector_get(&r.sampler.adaptive_sampler.op_samplers, 0);
    TEST_ASSERT_NOT_NULL(op_sampler);
    TEST_ASSERT_EQUAL_STRING("test-operation", op_sampler->operation_name);

    mock_http_server_destroy(&server);
    TEST_ASSERT_FALSE(jaeger_remotely_controlled_sampler_update(&r));
    TEST_ASSERT_EQUAL(jaeger_adaptive_sampler_type, r.sampler.type);
    TEST_ASSERT_EQUAL(
        1, jaeger_vector_length(&r.sampler.adaptive_sampler.op_samplers));
    op_sampler = jaeger_vector_get(&r.sampler.adaptive_sampler.op_samplers, 0);
    TEST_ASSERT_NOT_NULL(op_sampler);
    TEST_ASSERT_EQUAL_STRING("test-operation", op_sampler->operation_name);

    const jaeger_trace_id trace_id = {.high = 0, .low = 0};
    jaeger_vector tags;
    jaeger_vector_init(&tags, sizeof(jaeger_tag));
    ((jaeger_sampler*) &r)
        ->is_sampled((jaeger_sampler*) &r, &trace_id, "test-operation", &tags);

    JAEGERTRACINGC_VECTOR_FOR_EACH(&tags, jaeger_tag_destroy, jaeger_tag);
    jaeger_vector_destroy(&tags);
    ((jaeger_destructible*) &r)->destroy((jaeger_destructible*) &r);
}

static inline void test_sampler_choice()
{
    jaeger_sampler_choice choice;

#define FOR_EACH_SAMPLER_TEST(X)           \
    X(const)                               \
    X(probabilistic)                       \
    X(rate_limiting)                       \
    X(guaranteed_throughput_probabilistic) \
    X(adaptive)

#define CHECK_ASSIGN(sampler_type)                                     \
    do {                                                               \
        choice.type = jaeger_##sampler_type##_sampler_type;            \
        TEST_ASSERT_EQUAL(&choice.sampler_type##_sampler,              \
                          jaeger_sampler_choice_get_sampler(&choice)); \
    } while (0);

    FOR_EACH_SAMPLER_TEST(CHECK_ASSIGN)
}

void test_sampler()
{
    RUN_TEST(test_const_sampler);
    RUN_TEST(test_probabilistic_sampler);
    RUN_TEST(test_rate_limiting_sampler);
    RUN_TEST(test_guaranteed_throughput_probabilistic_sampler);
    RUN_TEST(test_adaptive_sampler);
    RUN_TEST(test_remotely_controlled_sampler);
    RUN_TEST(test_sampler_choice);
}
