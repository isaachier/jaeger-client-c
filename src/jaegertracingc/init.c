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

#include "jaegertracingc/init.h"
#include <pthread.h>
#include <stdlib.h>
#include <time.h>

static inline void init_lib()
{
    jaeger_alloc = jaeger_built_in_allocator();

#ifndef HAVE_RAND_R
    /* Set up global random seed */
    srand(time(NULL));
#endif /* HAVE_RAND_R */
}

void jaeger_init_lib(jaeger_allocator* alloc)
{
    static pthread_once_t is_initialized = PTHREAD_ONCE_INIT;
    const bool first_run = pthread_once(&is_initialized, init_lib);
    if (first_run && alloc != NULL) {
        jaeger_alloc = alloc;
    }
}
