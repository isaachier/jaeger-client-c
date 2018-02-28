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

#include <stdbool.h>
#include <stdlib.h>

#include <jansson.h>

static void* built_in_malloc(jaeger_allocator* alloc, size_t sz)
{
    (void) alloc;
#ifdef JAEGERTRACINGC_VERBOSE_ALLOC
    void* ptr = malloc(sz);
    printf("allocating %zu bytes at %p\n", sz, ptr);
    return ptr;
#else
    return malloc(sz);
#endif /* JAEGERTRACINGC_VERBOSE_ALLOC */
}

static void* built_in_realloc(jaeger_allocator* alloc, void* ptr, size_t sz)
{
    (void) alloc;
#ifdef JAEGERTRACINGC_VERBOSE_ALLOC
    void* new_ptr = realloc(ptr, sz);
    printf("reallocating %zu bytes at %p from %p\n", sz, new_ptr, ptr);
    return new_ptr;
#else
    return realloc(ptr, sz);
#endif /* JAEGERTRACINGC_VERBOSE_ALLOC */
}

static void built_in_free(jaeger_allocator* alloc, void* ptr)
{
    (void) alloc;
#ifdef JAEGERTRACINGC_VERBOSE_ALLOC
    printf("freeing memory at %p\n", ptr);
    free(ptr);
#else
    free(ptr);
#endif /* JAEGERTRACINGC_VERBOSE_ALLOC */
}

static void* null_malloc(jaeger_allocator* alloc, size_t size)
{
    (void) alloc;
    (void) size;
    return NULL;
}

static void* null_realloc(jaeger_allocator* alloc, void* ptr, size_t size)
{
    (void) alloc;
    (void) ptr;
    (void) size;
    return NULL;
}

static void null_free(jaeger_allocator* alloc, void* ptr)
{
    (void) alloc;
    (void) ptr;
}

#define BUILT_IN_ALLOCATOR_INIT                                   \
    {                                                             \
        .malloc = &built_in_malloc, .realloc = &built_in_realloc, \
        .free = &built_in_free                                    \
    }

jaeger_allocator* jaeger_built_in_allocator(void)
{
    static struct jaeger_allocator built_in_alloc = BUILT_IN_ALLOCATOR_INIT;
    return &built_in_alloc;
}

jaeger_allocator* jaeger_null_allocator(void)
{
    static struct jaeger_allocator null_alloc = {
        .malloc = &null_malloc, .realloc = &null_realloc, .free = &null_free};
    return &null_alloc;
}

void jaeger_set_allocator(jaeger_allocator* alloc)
{
    assert(alloc != NULL);
    *jaeger_get_allocator() = *alloc;
}

jaeger_allocator* jaeger_get_allocator(void)
{
    static jaeger_allocator shared_alloc = BUILT_IN_ALLOCATOR_INIT;

    /* Flag to indicate we have to call json_set_alloc_funcs. Not thread-safe,
     * but not a huge deal to call json_set_alloc_funcs multiple times.
     */
    static bool initialized = false;
    if (!initialized) {
        json_set_alloc_funcs(&jaeger_malloc, &jaeger_free);
        initialized = true;
    }
    return &shared_alloc;
}

void* jaeger_malloc(size_t sz)
{
    jaeger_allocator* alloc = jaeger_get_allocator();
    return alloc->malloc(alloc, sz);
}

void* jaeger_realloc(void* ptr, size_t sz)
{
    jaeger_allocator* alloc = jaeger_get_allocator();
    return alloc->realloc(alloc, ptr, sz);
}

void jaeger_free(void* ptr)
{
    jaeger_allocator* alloc = jaeger_get_allocator();
    alloc->free(alloc, ptr);
}
