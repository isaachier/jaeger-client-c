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
}
