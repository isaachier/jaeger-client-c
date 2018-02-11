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

#ifndef JAEGERTRACINGC_THREADING_H
#define JAEGERTRACINGC_THREADING_H

#include "jaegertracingc/common.h"

#ifdef JAEGERTRACINGC_MT
#include <pthread.h>

typedef pthread_mutex_t jaeger_mutex;

#define JAEGERTRACINGC_MUTEX_INIT PTHREAD_MUTEX_INITIALIZER

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

typedef pthread_once_t jaeger_once;
#define JAEGERTRACINGC_ONCE_INIT PTHREAD_ONCE_INIT

static inline int jaeger_do_once(jaeger_once* once, void (*init_routine)(void))
{
    return pthread_once(once, init_routine);
}

#else

typedef struct {
} jaeger_mutex;

#define JAEGERTRACINGC_MUTEX_INIT \
    {                             \
    }

static inline int jaeger_mutex_lock(jaeger_mutex* mutex)
{
    return 0;
}

static inline int jaeger_mutex_unlock(jaeger_mutex* mutex)
{
    return 0;
}

static inline int jaeger_mutex_destroy(jaeger_mutex* mutex)
{
    return 0;
}

typedef int jaeger_once;

#define JAEGERTRACINGC_ONCE_INIT 0

static inline int jaeger_do_once(jaeger_once* once, void (*init_routine)(void))
{
    if (!*once) {
        *once = 1;
        init_routine();
    }
}

#endif /* JAEGERTRACINGC_MT */

#ifdef __cplusplus
} /* extern C */
#endif /* __cplusplus */

#endif /* JAEGERTRACINGC_THREADING_H */
