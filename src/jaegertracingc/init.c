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

#include <jansson.h>
#include <pthread.h>
#include "jaegertracingc/alloc.h"
#include "jaegertracingc/logging.h"
#include "jaegertracingc/threading.h"

static jaeger_allocator* default_alloc = NULL;
static jaeger_logger* default_logger = NULL;

static inline void init_lib()
{
    default_alloc = jaeger_built_in_allocator();

    static jaeger_logger std_logger;
    jaeger_std_logger_init(&std_logger);
    default_logger = &std_logger;

    srand(time(NULL));

    /* Set Jansson allocation functions. */
    json_set_alloc_funcs(&jaeger_malloc, &jaeger_free);
}

void jaeger_init_lib(jaeger_logger* logger, jaeger_allocator* alloc)
{
    static jaeger_once is_initialized = JAEGERTRACINGC_ONCE_INIT;
    const int result = jaeger_do_once(&is_initialized, init_lib);
    if (result == 0) {
        if (alloc != NULL) {
            default_alloc = alloc;
        }
        if (logger != NULL) {
            default_logger = logger;
        }
    }
}

jaeger_allocator* jaeger_default_allocator()
{
    assert(default_alloc != NULL);
    return default_alloc;
}

jaeger_logger* jaeger_default_logger()
{
    assert(default_logger != NULL);
    return default_logger;
}
