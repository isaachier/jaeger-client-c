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

#ifndef JAEGERTRACINGC_THREADS_H
#define JAEGERTRACINGC_THREADS_H

#ifdef HAVE_PTHREAD
#include <pthread.h>
#endif /* HAVE_PTHREAD */

#include "jaegertracingc/common.h"

#ifdef HAVE_PTHREAD

#define JAEGER_MUTEX_INIT PTHREAD_MUTEX_INITIALIZER

typedef pthread_mutex_t jaeger_mutex;
typedef pthread_mutexattr_t jaeger_mutex_attr;

static inline int jaeger_mutex_init(jaeger_mutex* mutex,
                                    jaeger_mutex_attr* attr)
{
    return pthread_mutex_init(mutex, attr);
}

static inline int jaeger_mutex_lock(jaeger_mutex* mutex)
{
    return pthread_mutex_lock(mutex);
}

static inline int jaeger_mutex_unlock(jaeger_mutex* mutex)
{
    return pthread_mutex_unlock(mutex);
}

static inline int jaeger_mutex_destroy(jaeger_mutex* mutex)
{
    return pthread_mutex_destroy(mutex);
}

#else

#define JAEGER_MUTEX_INIT \
    {                     \
    }

typedef struct {
} jaeger_mutex;

typedef struct {
} jaeger_mutex_attr;

static inline int jaeger_mutex_init(jaeger_mutex* mutex,
                                    jaeger_mutex_attr* attr)
{
    (void) mutex;
    return 0;
}

static inline int jaeger_mutex_lock(jaeger_mutex* mutex)
{
    (void) mutex;
    return 0;
}

static inline int jaeger_mutex_unlock(jaeger_mutex* mutex)
{
    (void) mutex;
    return 0;
}

static inline int jaeger_mutex_destroy(jaeger_mutex* mutex)
{
    (void) mutex;
    return 0;
}

#endif /* HAVE_PTHREAD */

#endif /* JAEGERTRACINGC_THREADS_H */
