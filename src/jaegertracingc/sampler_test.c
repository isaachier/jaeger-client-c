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

void test_sampler()
{
    jaeger_const_sampler c;
    jaeger_const_sampler_init(&c, true);
    const char* operation_name = "test-operation";
    const jaeger_trace_id trace_id = (jaeger_trace_id){};
    TEST_ASSERT_TRUE(
        c.is_sampled((jaeger_sampler*) &c, &trace_id, operation_name, NULL));
    c.close((jaeger_sampler*) &c);

    jaeger_const_sampler_init(&c, false);
    TEST_ASSERT_FALSE(
        c.is_sampled((jaeger_sampler*) &c, &trace_id, operation_name, NULL));
    c.close((jaeger_sampler*) &c);
}
