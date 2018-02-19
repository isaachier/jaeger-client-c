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

#include "jaegertracingc/vector.h"
#include "jaegertracingc/tag.h"
#include "unity.h"

static inline void* null_malloc(jaeger_allocator* alloc, size_t size)
{
    (void) alloc;
    (void) size;
    return NULL;
}

static inline void*
null_realloc(jaeger_allocator* alloc, void* ptr, size_t size)
{
    (void) alloc;
    (void) ptr;
    (void) size;
    return NULL;
}

static inline void null_free(jaeger_allocator* alloc, void* ptr)
{
    (void) alloc;
    (void) ptr;
}

void test_vector()
{
    /* Most of vector is covered in tag_test. This test covers only edge cases.
     */
    jaeger_allocator alloc = {
        .malloc = &null_malloc, .realloc = &null_realloc, .free = &null_free};
    jaeger_vector vec;
    jaeger_logger* logger = jaeger_null_logger();

    TEST_ASSERT_FALSE(jaeger_vector_init(&vec, sizeof(char), &alloc, logger));

    TEST_ASSERT_TRUE(jaeger_vector_init(&vec, sizeof(char), NULL, logger));
    vec.alloc = &alloc;
    while (jaeger_vector_length(&vec) < JAEGERTRACINGC_VECTOR_INIT_CAPACITY) {
        TEST_ASSERT_NOT_NULL(jaeger_vector_append(&vec, logger));
    }
    TEST_ASSERT_NULL(jaeger_vector_append(&vec, logger));
    jaeger_vector_remove(&vec, 0, logger);
    vec.alloc = jaeger_built_in_allocator();
    TEST_ASSERT_NULL(
        jaeger_vector_get(&vec, jaeger_vector_length(&vec), logger));
    jaeger_vector_destroy(&vec);

    TEST_ASSERT_TRUE(
        jaeger_vector_init(&vec, sizeof(jaeger_tag), NULL, logger));
    vec.alloc = &alloc;
    jaeger_tag tag = JAEGERTRACINGC_TAG_INIT;
    tag.value_case = JAEGERTRACINGC_TAG_TYPE(LONG);
    tag.long_value = 0;
    TEST_ASSERT_TRUE(jaeger_vector_extend(
        &vec, 0, JAEGERTRACINGC_VECTOR_INIT_CAPACITY, logger));
    TEST_ASSERT_FALSE(jaeger_tag_vector_append(&vec, &tag, logger));
    vec.alloc = jaeger_built_in_allocator();
    jaeger_vector_destroy(&vec);
}
