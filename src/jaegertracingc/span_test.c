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

#include "jaegertracingc/span.h"
#include "unity.h"

static const jaeger_key_value baggage[] = {
    {.key = "key1", .value = "value1"},
    {.key = "key2", .value = "value2"},
    {.key = "key3", .value = "value3"},
};

static opentracing_bool
visit_baggage(void* arg, const char* key, const char* value)
{
    const jaeger_key_value* kv = &baggage[*(int*) arg];
    TEST_ASSERT_EQUAL_STRING(kv->key, key);
    TEST_ASSERT_EQUAL_STRING(kv->value, value);
    (*(int*) arg)++;
    return opentracing_true;
}

void test_span()
{
    typedef struct test_case {
        const char* input;
        bool success;
    } test_case;
    const test_case cases[] = {
        {"abcd", false},
        {"x:1:1:1", false},
        {"1:x:1:1", false},
        {"1:1:x:1", false},
        {"1:1:1:x", false},
        {"01234567890123456789012345678901234:1:1:1", false},
        {"01234567890123456789012345678901:1:1:1", true}};
    const int num_cases = sizeof(cases) / sizeof(cases[0]);
    for (int i = 0; i < num_cases; i++) {
        const test_case test = cases[i];
        jaeger_span_context span = JAEGERTRACINGC_SPAN_CONTEXT_INIT;
        const bool success = jaeger_span_context_scan(&span, test.input);
        TEST_ASSERT_EQUAL_MESSAGE(test.success, success, test.input);
    }

    jaeger_key_value_destroy(NULL);
    jaeger_log_record_destroy(NULL);
    jaeger_span_ref_destroy(NULL);
    jaeger_span_context_destroy(NULL);
    jaeger_span_destroy(NULL);

    jaeger_span span = JAEGERTRACINGC_SPAN_INIT;
    TEST_ASSERT_TRUE(jaeger_span_init(&span));

    TEST_ASSERT_FALSE(jaeger_span_context_is_valid(&span.context));
    TEST_ASSERT_FALSE(
        jaeger_span_context_is_debug_id_container_only(&span.context));

    for (int i = 0, len = sizeof(baggage) / sizeof(baggage[0]); i < len; i++) {
        ((opentracing_span*) &span)
            ->set_baggage_item(
                ((opentracing_span*) &span), baggage[i].key, baggage[i].value);
    }
    int arg = 0;
    ((opentracing_span_context*) &span.context)
        ->foreach_baggage_item(
            ((opentracing_span_context*) &span.context), &visit_baggage, &arg);
    ((opentracing_destructible*) &span)
        ->destroy((opentracing_destructible*) &span);
}
