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

static void* default_malloc(jaeger_allocator* alloc, size_t sz)
{
    (void) alloc;
    return malloc(sz);
}

static void* default_realloc(jaeger_allocator* alloc, void* ptr, size_t sz)
{
    (void) alloc;
    return realloc(ptr, sz);
}

static void default_free(jaeger_allocator* alloc, void* ptr)
{
    (void) alloc;
    free(ptr);
}

struct jaeger_allocator jaeger_default_alloc = {.malloc = &default_malloc,
                                        .realloc = &default_realloc,
                                        .free = &default_free };

void* jaeger_default_alloc_malloc(size_t sz)
{
    return jaeger_default_alloc.malloc(&jaeger_default_alloc, sz);
}

void* jaeger_default_alloc_realloc(void* ptr, size_t sz)
{
    return jaeger_default_alloc.realloc(&jaeger_default_alloc, ptr, sz);
}

void jaeger_default_alloc_free(void* ptr)
{
    return jaeger_default_alloc.free(&jaeger_default_alloc, ptr);
}
