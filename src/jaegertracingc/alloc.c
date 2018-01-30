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

#include <stdlib.h>

static void* global_malloc(jaeger_allocator* alloc, size_t sz)
{
    (void) alloc;
    return malloc(sz);
}

static void* global_realloc(jaeger_allocator* alloc, void* ptr, size_t sz)
{
    (void) alloc;
    return realloc(ptr, sz);
}

static void global_free(jaeger_allocator* alloc, void* ptr)
{
    (void) alloc;
    free(ptr);
}

static struct jaeger_allocator global_alloc = {.malloc = &global_malloc,
                                        .realloc = &global_realloc,
                                        .free = &global_free };
jaeger_allocator* jaeger_global_alloc = &global_alloc;

void* jaeger_global_alloc_malloc(size_t sz)
{
    return jaeger_global_alloc->malloc(jaeger_global_alloc, sz);
}

void* jaeger_global_alloc_realloc(void* ptr, size_t sz)
{
    return jaeger_global_alloc->realloc(jaeger_global_alloc, ptr, sz);
}

void jaeger_global_alloc_free(void* ptr)
{
    return jaeger_global_alloc->free(jaeger_global_alloc, ptr);
}
