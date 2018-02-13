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

#include "jaegertracingc/trace_id.h"
#include "unity.h"

void test_trace_id()
{
    const jaeger_trace_id trace_ids[] = {{.high = 0, .low = 0},
                                         {.high = 1, .low = 2},
                                         {.high = UINT_MAX, .low = 0}};
    const int num_trace_ids = sizeof(trace_ids) / sizeof(trace_ids[0]);
    for (int i = 0; i < num_trace_ids; i++) {
        const jaeger_trace_id trace_id = trace_ids[i];
        char buffer[JAEGERTRACINGC_TRACE_ID_MAX_STR_LEN];
        const int str_len =
            jaeger_trace_id_format(&trace_id, buffer, sizeof(buffer));
        TEST_ASSERT_LESS_THAN(sizeof(buffer), str_len);
        jaeger_trace_id decoded_trace_id = JAEGERTRACINGC_TRACE_ID_INIT;
        TEST_ASSERT_EQUAL(
            buffer + str_len,
            jaeger_trace_id_scan(&decoded_trace_id, buffer, buffer + str_len));
        TEST_ASSERT_EQUAL(trace_id.high, decoded_trace_id.high);
        TEST_ASSERT_EQUAL(trace_id.low, decoded_trace_id.low);
    }

    jaeger_trace_id decoded_trace_id = JAEGERTRACINGC_TRACE_ID_INIT;
    const char* bad_trace_id_str = "abcfg";
    const char* bad_iter = bad_trace_id_str + 4;
    TEST_ASSERT_EQUAL(
        bad_iter,
        jaeger_trace_id_scan(&decoded_trace_id,
                             bad_trace_id_str,
                             bad_trace_id_str + strlen(bad_trace_id_str)));
    bad_trace_id_str = "g0000000000000000";
    bad_iter = bad_trace_id_str;
    TEST_ASSERT_EQUAL(
        bad_iter,
        jaeger_trace_id_scan(&decoded_trace_id,
                             bad_trace_id_str,
                             bad_trace_id_str + strlen(bad_trace_id_str)));
}
