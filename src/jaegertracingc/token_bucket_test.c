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

#include "jaegertracingc/token_bucket.h"
#include <time.h>
#include "jaegertracingc/alloc.h"
#include "jaegertracingc/duration.h"
#include "unity.h"

#define NS_PER_S JAEGERTRACINGC_NANOSECONDS_PER_SECOND

void test_token_bucket()
{
    const double credits_per_second = 10;
    const double max_balance = 3;
    jaeger_token_bucket tok;
    jaeger_token_bucket_init(&tok, credits_per_second, max_balance);
    bool result = jaeger_token_bucket_check_credit(&tok, max_balance);
    TEST_ASSERT_TRUE(result);
    jaeger_duration sleep_time = {.tv_sec = 0, .tv_nsec = NS_PER_S * 0.01};
    jaeger_duration rem_time = {.tv_sec = 0, .tv_nsec = 0};
    nanosleep(&sleep_time, &rem_time);
    jaeger_duration interval;
    result = jaeger_duration_subtract(&sleep_time, &rem_time, &interval);
    TEST_ASSERT_TRUE(result);
    const double expected_credits =
        credits_per_second * interval.tv_sec +
        credits_per_second * interval.tv_nsec / NS_PER_S;
    result = jaeger_token_bucket_check_credit(&tok, expected_credits);
    TEST_ASSERT_TRUE(result);
    result = jaeger_token_bucket_check_credit(&tok, expected_credits);
    TEST_ASSERT_FALSE(result);
}
