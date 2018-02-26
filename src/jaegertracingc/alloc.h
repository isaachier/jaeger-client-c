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

#ifndef JAEGERTRACINGC_ALLOC_H
#define JAEGERTRACINGC_ALLOC_H

#include "jaegertracingc/common.h"
#include "jaegertracingc/init.h"
#include "jaegertracingc/logging.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/** Interface to override default allocator. */
typedef struct jaeger_allocator {
    /** malloc function override.
     * @param alloc The allocator instance.
     * @param sz The size to allocate.
     * @return Pointer to allocated memory on success, NULL otherwise.
     */
    void* (*malloc)(struct jaeger_allocator* alloc, size_t sz);

    /** realloc function override.
     * @param alloc The allocator instance.
     * @param ptr Pointer to previously allocated memory.
     * @param sz The new size to allocate.
     * @return Pointer to new allocated memory on success, NULL otherwise.
     */
    void* (*realloc)(struct jaeger_allocator* alloc, void* ptr, size_t sz);

    /** free function override.
     * @param alloc The allocator instance.
     * @param ptr Pointer to he allocated memory to free.
     */
    void (*free)(struct jaeger_allocator* alloc, void* ptr);
} jaeger_allocator;

/** Wrapper for built-in allocation functions.
 * @return Shared instance of built-in allocator. DO NOT MODIFY MEMBERS!
 */
jaeger_allocator* jaeger_built_in_allocator();

/** Wrapper for null allocation functions. Never allocates memory,
 * all function are no-ops. Used for testing.
 * @return Shared instance of null allocator. DO NOT MODIFY MEMBERS!
 */
jaeger_allocator* jaeger_null_allocator();

/** Wrapper for installed allocator's malloc.
 * @param sz Size to allocate.
 * @return Result of call to default allocator's malloc function.
 * @see jaeger_init_lib()
 */
static inline void* jaeger_malloc(size_t sz)
{
    return jaeger_default_allocator()->malloc(jaeger_default_allocator(), sz);
}

/** Wrapper for installed allocator's realloc.
 * @param ptr Pointer to previously allocated memory.
 * @param sz New size to allocate.
 * @return Result of call to default allocator's realloc function.
 * @see jaeger_init_lib()
 */
static inline void* jaeger_realloc(void* ptr, size_t sz)
{
    return jaeger_default_allocator()->realloc(
        jaeger_default_allocator(), ptr, sz);
}

/** Wrapper for installed allocator's free.
 * @param ptr Pointer to allocated memory to free.
 * @see jaeger_init_lib()
 */
static inline void jaeger_free(void* ptr)
{
    jaeger_default_allocator()->free(jaeger_default_allocator(), ptr);
}

/** Duplicates string using provided allocator, logging to logger on failure.
 * @param str String to duplicate.
 * @param alloc Allocator to use.
 * @param logger Logger to log to on failure.
 * @return New string copy on success, NULL on failure.
 */
static inline char* jaeger_strdup_alloc(const char* str,
                                        jaeger_allocator* alloc,
                                        jaeger_logger* logger)
{
    assert(str != NULL);
    if (alloc == NULL) {
        alloc = jaeger_built_in_allocator();
    }
    const int size = strlen(str) + 1;
    char* copy = (char*) alloc->malloc(alloc, size);
    if (copy == NULL) {
        logger->error(logger, "Cannot allocate string copy, size = %d", size);
        return NULL;
    }
    memcpy(copy, str, size);
    return copy;
}

/** Like jaeger_strdup_alloc, but uses default allocator.
 * @param str String to duplicate.
 * @param logger Logger to log to on failure.
 * @return Result of jaeger_strdup using default allocator.
 * @see jaeger_init_lib().
 * @see jaeger_strdup_alloc().
 */
static inline char* jaeger_strdup(const char* str, jaeger_logger* logger)
{
    return jaeger_strdup_alloc(str, NULL, logger);
}

#ifdef __cplusplus
} /* extern C */
#endif /* __cplusplus */

#endif /* JAEGERTRACINGC_ALLOC_H */
