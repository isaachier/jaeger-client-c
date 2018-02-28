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

#include "jaegertracingc/alloc.h"

#include <time.h>

#include "jaegertracingc/logging.h"
#include "unity.h"

void test_alloc()
{
    void* mem = jaeger_malloc(2);
    mem = jaeger_realloc(mem, 4);
    jaeger_free(mem);

    jaeger_set_allocator(jaeger_null_allocator());
    char* str = jaeger_strdup("hello world");
    TEST_ASSERT_NULL(str);
    /* Calling free even though null to improve coverage of null allocator. */
    jaeger_free(str);
    jaeger_set_allocator(jaeger_built_in_allocator());
}
