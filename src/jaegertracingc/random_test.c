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

#include "jaegertracingc/random.h"
#include "jaegertracingc/threading.h"
#include "unity.h"

#define NUM_THREADS 5

static void* random_func(void* arg)
{
    TEST_ASSERT_NOT_NULL(arg);
    int64_t* x = arg;
    jaeger_logger* logger = jaeger_null_logger();
    *x = random64(logger);
    return NULL;
}

void test_random()
{
    int64_t random_numbers[NUM_THREADS] = {0};
    jaeger_thread threads[NUM_THREADS];
    for (int i = 0; i < NUM_THREADS; i++) {
        TEST_ASSERT_EQUAL(
            0,
            jaeger_thread_init(&threads[i], &random_func, &random_numbers[i]));
    }
    for (int i = 0; i < NUM_THREADS; i++) {
        jaeger_thread_join(threads[i], NULL);
    }
}
