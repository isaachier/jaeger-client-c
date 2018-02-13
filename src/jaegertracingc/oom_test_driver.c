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

#include "alloc.h"
#include "init.h"
#include "unit_test_driver.h"

static void* oom_malloc(jaeger_allocator* context, size_t sz)
{
    (void) context;
    (void) sz;
    return NULL;
}

static void* oom_realloc(jaeger_allocator* context, void* ptr, size_t sz)
{
    (void) context;
    (void) sz;
    return NULL;
}

static void oom_free(jaeger_allocator* context, void* ptr)
{
    (void) context;
    free(ptr);
}

int main()
{
    jaeger_allocator alloc = {
        .malloc = &oom_malloc, .realloc = &oom_realloc, .free = &oom_free};
    jaeger_init_lib(&alloc);
    run_tests();
    return 0;
}
