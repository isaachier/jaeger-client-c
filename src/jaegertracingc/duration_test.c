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

#include "jaegertracingc/duration.h"
#include "unity.h"
#include <time.h>

#define NS_PER_S JAEGERTRACINGC_NANOSECONDS_PER_SECOND

void test_duration()
{
    {
        jaeger_duration x = {.tv_sec = 1, .tv_nsec = 0 };
        jaeger_duration y = {.tv_sec = 0, .tv_nsec = NS_PER_S * 0.5 };
        jaeger_duration result;
        jaeger_duration_subtract(&x, &y, &result);
        TEST_ASSERT_EQUAL(0, result.tv_sec);
        TEST_ASSERT_EQUAL(NS_PER_S * 0.5, result.tv_nsec);
    }

    {
        jaeger_duration x = {
            .tv_sec = 0,
            .tv_nsec = NS_PER_S * 1.1 /* Testing edge case for coverage */
        };
        jaeger_duration y = {.tv_sec = 0, .tv_nsec = NS_PER_S * 0.0 };
        jaeger_duration result;
        jaeger_duration_subtract(&x, &y, &result);
        TEST_ASSERT_EQUAL(1, result.tv_sec);
        TEST_ASSERT_EQUAL(NS_PER_S * 0.1, result.tv_nsec);
    }
}
