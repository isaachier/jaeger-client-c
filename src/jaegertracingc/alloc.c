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

#ifdef JAEGERTRACINGC_VERBOSE_ALLOC
#include <stdio.h>
#endif /* JAEGERTRACINGC_VERBOSE_ALLOC */
#include <stdlib.h>

static void* global_malloc(jaeger_allocator* alloc, size_t sz)
{
    (void)alloc;
#ifdef JAEGERTRACINGC_VERBOSE_ALLOC
    void* ptr = malloc(sz);
    printf("allocating %zu bytes at %p\n", sz, ptr);
    return ptr;
#else
    return malloc(sz);
#endif /* JAEGERTRACINGC_VERBOSE_ALLOC */
}

static void* global_realloc(jaeger_allocator* alloc, void* ptr, size_t sz)
{
    (void)alloc;
#ifdef JAEGERTRACINGC_VERBOSE_ALLOC
    void* new_ptr = realloc(ptr, sz);
    printf("reallocating %zu bytes at %p from %p\n", sz, new_ptr, ptr);
    return new_ptr;
#else
    return realloc(ptr, sz);
#endif /* JAEGERTRACINGC_VERBOSE_ALLOC */
}

static void global_free(jaeger_allocator* alloc, void* ptr)
{
    (void)alloc;
#ifdef JAEGERTRACINGC_VERBOSE_ALLOC
    printf("freeing memory at %p\n", ptr);
    free(ptr);
#else
    free(ptr);
#endif /* JAEGERTRACINGC_VERBOSE_ALLOC */
}

static struct jaeger_allocator global_alloc = { .malloc = &global_malloc,
                                                .realloc = &global_realloc,
                                                .free = &global_free };
jaeger_allocator* jaeger_global_alloc = NULL;

jaeger_allocator* jaeger_built_in_allocator() { return &global_alloc; }
