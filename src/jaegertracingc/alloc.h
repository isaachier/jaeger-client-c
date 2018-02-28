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

/**
 * @file
 * Allocator struct and functions.
 */

#ifndef JAEGERTRACINGC_ALLOC_H
#define JAEGERTRACINGC_ALLOC_H

#include <assert.h>
#include <stdint.h>
#include <string.h>

#include "jaegertracingc/logging.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/** Interface to override default allocator. */
typedef struct jaeger_allocator {
    /**
     * malloc function override.
     * @param alloc Allocator instance.
     * @param sz Size to allocate.
     * @return Pointer to allocated memory on success, NULL otherwise.
     */
    void* (*malloc)(struct jaeger_allocator* alloc, size_t sz);

    /**
     * realloc function override.
     * @param alloc Allocator instance.
     * @param ptr Pointer to previously allocated memory.
     * @param sz New size to allocate.
     * @return Pointer to new allocated memory on success, NULL otherwise.
     */
    void* (*realloc)(struct jaeger_allocator* alloc, void* ptr, size_t sz);

    /**
     * free function override.
     * @param alloc Allocator instance.
     * @param ptr Pointer to he allocated memory to free.
     */
    void (*free)(struct jaeger_allocator* alloc, void* ptr);
} jaeger_allocator;

/**
 * Wrapper for built-in allocation functions.
 * @return Shared instance of built-in allocator. DO NOT MODIFY MEMBERS!
 */
jaeger_allocator* jaeger_built_in_allocator(void);

/**
 * Wrapper for null allocation functions. Never allocates memory,
 * all function are no-ops. Used for testing.
 * @return Shared instance of null allocator. DO NOT MODIFY MEMBERS!
 */
jaeger_allocator* jaeger_null_allocator(void);

/**
 * Set the installed allocator.
 * @param alloc Allocator instance.
 */
void jaeger_set_allocator(jaeger_allocator* alloc);

/**
 * Get the installed allocator.
 * @return alloc Allocator instance.
 */
jaeger_allocator* jaeger_get_allocator(void);

/**
 * Returns result of call to malloc using installed allocator.
 * @param sz Size to be allocated.
 * @return Address to allocated memory on success, NULL otherwise.
 */
void* jaeger_malloc(size_t sz);

/**
 * Returns result of call to realloc using installed allocator.
 * @param ptr Pointer to previously allocated memory.
 * @param sz New size to allocate.
 * @return Address to allocated memory on success, NULL otherwise.
 */
void* jaeger_realloc(void* ptr, size_t sz);

/**
 * Calls free using installed allocator.
 * @param ptr Allocated address.
 */
void jaeger_free(void* ptr);

/**
 * Duplicates string using provided allocator, logging to logger on failure.
 * @param str String to duplicate.
 * @return New string copy on success, NULL on failure.
 */
static inline char* jaeger_strdup(const char* str)
{
    assert(str != NULL);
    const int size = strlen(str) + 1;
    char* copy = (char*) jaeger_malloc(size);
    if (copy == NULL) {
        jaeger_log_error("Cannot allocate string copy, size = %d", size);
        return NULL;
    }
    memcpy(copy, str, size);
    return copy;
}

#ifdef __cplusplus
} /* extern C */
#endif /* __cplusplus */

#endif /* JAEGERTRACINGC_ALLOC_H */
