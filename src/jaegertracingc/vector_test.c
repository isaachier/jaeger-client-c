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

#include "jaegertracingc/tag.h"
#include "jaegertracingc/vector.h"
#include "unity.h"

void test_vector()
{
    /* Most of vector is covered in tag_test. This test covers only edge cases.
     */
    jaeger_vector vec;

    jaeger_set_allocator(jaeger_null_allocator());
    TEST_ASSERT_FALSE(jaeger_vector_init(&vec, sizeof(char)));

    jaeger_set_allocator(jaeger_built_in_allocator());
    TEST_ASSERT_TRUE(jaeger_vector_init(&vec, sizeof(char)));
    jaeger_set_allocator(jaeger_null_allocator());
    while (jaeger_vector_length(&vec) < JAEGERTRACINGC_VECTOR_INIT_CAPACITY) {
        TEST_ASSERT_NOT_NULL(jaeger_vector_append(&vec));
    }
    TEST_ASSERT_NULL(jaeger_vector_append(&vec));
    jaeger_vector_remove(&vec, 0);
    TEST_ASSERT_NULL(jaeger_vector_get(&vec, jaeger_vector_length(&vec)));
    jaeger_set_allocator(jaeger_built_in_allocator());
    jaeger_vector_destroy(&vec);

    TEST_ASSERT_TRUE(jaeger_vector_init(&vec, sizeof(jaeger_tag)));
    jaeger_tag tag = JAEGERTRACINGC_TAG_INIT;
    tag.v_type = JAEGERTRACINGC_TAG_TYPE(INT64);
    tag.v_int64 = 0;
    TEST_ASSERT_TRUE(
        jaeger_vector_extend(&vec, 0, JAEGERTRACINGC_VECTOR_INIT_CAPACITY));
    jaeger_set_allocator(jaeger_null_allocator());
    TEST_ASSERT_FALSE(jaeger_tag_vector_append(&vec, &tag));
    jaeger_set_allocator(jaeger_built_in_allocator());
    jaeger_vector_destroy(&vec);
}
