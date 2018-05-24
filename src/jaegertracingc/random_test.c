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

#include "jaegertracingc/internal/random.h"
#include "jaegertracingc/random.h"

#include "unity.h"

#include "jaegertracingc/alloc.h"
#include "jaegertracingc/threading.h"

#define NUM_THREADS 5

static void* random_func(void* arg)
{
    TEST_ASSERT_NOT_NULL(arg);
    int64_t* x = arg;
    *x = jaeger_random64();
    return NULL;
}

void test_random()
{
    /* Test allocation failure. */
    jaeger_set_allocator(jaeger_null_allocator());
    TEST_ASSERT_EQUAL(0, jaeger_random64());
    jaeger_set_allocator(jaeger_built_in_allocator());

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

    uint64_t seed[NUM_UINT64_IN_SEED];
    memset(seed, 0, sizeof(seed));
    read_random_seed(seed, sizeof(seed), "bad-path");
    TEST_ASSERT_EQUAL(0, seed[0]);
    TEST_ASSERT_EQUAL(0, seed[1]);

    FILE* f = fopen("good-path", "w+");
    TEST_ASSERT_NOT_NULL(f);
    const uint64_t seed_values[NUM_UINT64_IN_SEED] = {0x0123456789ABCDEF, 0};
    TEST_ASSERT_EQUAL(1, fwrite(seed_values, sizeof(seed_values[0]), 1, f));
    fclose(f);
    read_random_seed(seed, sizeof(seed), "good-path");
    TEST_ASSERT_EQUAL_HEX64(seed_values[0], seed[0]);
    TEST_ASSERT_EQUAL_HEX64(seed_values[1], seed[1]);
    remove("good-path");
}
