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

    for (size_t i = 0; i < num_insertions; i++) {
        char key[buffer_size];
        random_string(key, buffer_size);
        char value[buffer_size];
        random_string(value, buffer_size);

        TEST_ASSERT_TRUE(jaeger_hashtable_put(&hashtable, key, value));
        const jaeger_key_value* kv = jaeger_hashtable_find(&hashtable, key);
        TEST_ASSERT_NOT_NULL(kv);
        TEST_ASSERT_EQUAL_STRING(key, kv->key);
        TEST_ASSERT_EQUAL_STRING(value, kv->value);
    }

    jaeger_hashtable_destroy(&hashtable);
}
