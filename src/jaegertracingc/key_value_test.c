/*
 * Copyright (c) 2018 The Jaeger Authors.
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

#include "jaegertracingc/key_value.h"

#include "unity.h"

#include "jaegertracingc/alloc.h"

typedef struct counted_allocator {
    jaeger_allocator base;
    int counter;
} counted_allocator;

static void* counted_allocator_malloc(jaeger_allocator* alloc, size_t sz)
{
    counted_allocator* counted_alloc = (counted_allocator*) alloc;
    counted_alloc->counter++;
    if (counted_alloc->counter == 1) {
        return malloc(sz);
    }
    return NULL;
}

void test_key_value()
{
    counted_allocator alloc;
    jaeger_allocator* old_alloc = jaeger_get_allocator();
    ((jaeger_allocator*) &alloc)->malloc = &counted_allocator_malloc;
    ((jaeger_allocator*) &alloc)->realloc = old_alloc->realloc;
    ((jaeger_allocator*) &alloc)->free = old_alloc->free;
    alloc.counter = 0;
    jaeger_set_allocator((jaeger_allocator*) &alloc);
    jaeger_key_value kv;
    TEST_ASSERT_FALSE(jaeger_key_value_init(&kv, "hello", "world"));
    TEST_ASSERT_NULL(kv.key);
    TEST_ASSERT_NULL(kv.value);

    jaeger_set_allocator(old_alloc);
    jaeger_key_value copy_kv;
    TEST_ASSERT_TRUE(jaeger_key_value_init(&kv, "hello", "world"));
    TEST_ASSERT_TRUE(jaeger_key_value_copy(&copy_kv, &kv));
    jaeger_key_value_destroy(&kv);
    jaeger_key_value_destroy(&copy_kv);
}
