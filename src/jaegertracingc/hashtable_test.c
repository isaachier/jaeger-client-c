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

#include "jaegertracingc/hashtable.h"

#include "unity.h"

#include "jaegertracingc/internal/test_helpers.h"

void test_hashtable()
{
    enum { num_insertions = 100, buffer_size = 16 };

    jaeger_hashtable hashtable;
    TEST_ASSERT_TRUE(jaeger_hashtable_init(&hashtable));

    char key[buffer_size];
    char value[buffer_size];

    for (size_t i = 0; i < num_insertions; i++) {
        random_string(key, buffer_size);
        random_string(value, buffer_size);

        TEST_ASSERT_TRUE(jaeger_hashtable_put(&hashtable, key, value));
        const jaeger_key_value* kv = jaeger_hashtable_find(&hashtable, key);
        TEST_ASSERT_NOT_NULL(kv);
        TEST_ASSERT_EQUAL_STRING(key, kv->key);
        TEST_ASSERT_EQUAL_STRING(value, kv->value);

        char value_replacement[buffer_size];
        random_string(value_replacement, buffer_size);
        static bool test_oom_replace = false;
        if (rand() / RAND_MAX > 0.95 ||
            (i == num_insertions - 1 && !test_oom_replace)) {
            test_oom_replace = true;
            /* Test replace memory failure. */
            jaeger_set_allocator(jaeger_null_allocator());
            TEST_ASSERT_FALSE(
                jaeger_hashtable_put(&hashtable, key, value_replacement));
            jaeger_set_allocator(jaeger_built_in_allocator());
        }
        else {
            TEST_ASSERT_TRUE(
                jaeger_hashtable_put(&hashtable, key, value_replacement));
            kv = jaeger_hashtable_find(&hashtable, key);
            TEST_ASSERT_NOT_NULL(kv);
            TEST_ASSERT_EQUAL_STRING(key, kv->key);
            TEST_ASSERT_EQUAL_STRING(value_replacement, kv->value);
        }

        TEST_ASSERT_EQUAL(i + 1, hashtable.size);
    }

    /* Test insert memory failure. */
    random_string(key, buffer_size);
    random_string(value, buffer_size);
    jaeger_set_allocator(jaeger_null_allocator());
    TEST_ASSERT_FALSE(jaeger_hashtable_put(&hashtable, key, value));
    TEST_ASSERT_EQUAL(num_insertions, hashtable.size);
    jaeger_set_allocator(jaeger_built_in_allocator());
    const jaeger_key_value* kv = jaeger_hashtable_find(&hashtable, key);
    TEST_ASSERT_NULL(kv);

    jaeger_hashtable_clear(&hashtable);
    TEST_ASSERT_EQUAL(0, hashtable.size);

    jaeger_hashtable_destroy(&hashtable);
    jaeger_hashtable_destroy(NULL);

    /* Test rehash memory failure. */
    TEST_ASSERT_TRUE(jaeger_hashtable_init(&hashtable));
    for (size_t i = 0; (i + 1) / (1 << JAEGERTRACINGC_HASHTABLE_INIT_ORDER) <
                       JAEGERTRACINGC_HASHTABLE_THRESHOLD;
         i++) {
        TEST_ASSERT_TRUE(jaeger_hashtable_put(&hashtable, key, value));
        random_string(key, buffer_size);
        random_string(value, buffer_size);
    }
    random_string(key, buffer_size);
    random_string(value, buffer_size);
    jaeger_set_allocator(jaeger_null_allocator());
    TEST_ASSERT_FALSE(jaeger_hashtable_put(&hashtable, key, value));
    TEST_ASSERT_LESS_THAN(JAEGERTRACINGC_HASHTABLE_THRESHOLD,
                          hashtable.size /
                              (1 << JAEGERTRACINGC_HASHTABLE_INIT_ORDER));
    jaeger_set_allocator(jaeger_built_in_allocator());

    jaeger_hashtable_destroy(&hashtable);

    /* Test init memory failure. */
    jaeger_set_allocator(jaeger_null_allocator());
    TEST_ASSERT_FALSE(jaeger_hashtable_init(&hashtable));
    jaeger_set_allocator(jaeger_built_in_allocator());

    /* Test minimal size. */
    TEST_ASSERT_EQUAL_HEX(0x100, 1 << jaeger_hashtable_minimal_order(0xf0));
    TEST_ASSERT_EQUAL_HEX(0x10, 1 << jaeger_hashtable_minimal_order(0x8));
}
