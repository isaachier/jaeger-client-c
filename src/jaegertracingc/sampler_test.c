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

#define SET_UP_SAMPLER_TEST()                      \
    jaeger_tag_list tags;                          \
    jaeger_tag_list_init(&tags);                   \
    const char* operation_name = "test-operation"; \
    const jaeger_trace_id trace_id = JAEGERTRACING__PROTOBUF__TRACE_ID__INIT

#define TEAR_DOWN_SAMPLER_TEST(sampler)        \
    sampler.close((jaeger_sampler*) &sampler); \
    jaeger_tag_list_destroy(&tags)

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
    TEST_ASSERT_EQUAL(0, tags.tags[1].double_value);

    TEAR_DOWN_SAMPLER_TEST(p);
}
