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
 * Threading utilities. Should work on single-threaded builds if
 * JAEGERTRACINGC_MT is not defined.
 */

#ifndef JAEGERTRACINGC_THREADING_H
#define JAEGERTRACINGC_THREADING_H

#include "jaegertracingc/common.h"

#ifdef JAEGERTRACINGC_MT
#include <errno.h>
#include <pthread.h>
#include <sched.h>

typedef pthread_t jaeger_thread;
typedef pthread_mutex_t jaeger_mutex;
typedef pthread_cond_t jaeger_cond;
typedef pthread_once_t jaeger_once;

typedef struct jaeger_thread_local {
    pthread_key_t key;
    bool initialized;
} jaeger_thread_local;

#define JAEGERTRACINGC_MUTEX_INIT PTHREAD_MUTEX_INITIALIZER
#define JAEGERTRACINGC_COND_INIT PTHREAD_COND_INITIALIZER
#define JAEGERTRACINGC_ONCE_INIT PTHREAD_ONCE_INIT

#else

typedef struct {
    void* return_value;
} jaeger_thread;

typedef struct {
    bool locked;
} jaeger_mutex;

typedef struct {
    bool signal;
} jaeger_cond;

typedef struct {
    bool has_run;
} jaeger_once;

typedef struct jaeger_thread_local {
    jaeger_destructible* value;
} jaeger_thread_local;

#define JAEGERTRACINGC_MUTEX_INIT \
    {                             \
        .locked = false           \
    }
#define JAEGERTRACINGC_COND_INIT \
    {                            \
        .signal = false          \
    }
#define JAEGERTRACINGC_ONCE_INIT \
    {                            \
        .has_run = false         \
    }

#endif /* JAEGERTRACINGC_MT */

int jaeger_thread_init(jaeger_thread* thread,
                       void* (*start_routine)(void*),
                       void* arg);

int jaeger_thread_join(jaeger_thread thread, void** return_value);

int jaeger_yield(void);

int jaeger_mutex_lock(jaeger_mutex* mutex);

int jaeger_mutex_try_lock(jaeger_mutex* mutex);

int jaeger_mutex_unlock(jaeger_mutex* mutex);

int jaeger_mutex_destroy(jaeger_mutex* mutex);

int jaeger_cond_destroy(jaeger_cond* cond);

int jaeger_cond_signal(jaeger_cond* cond);

int jaeger_cond_wait(jaeger_cond* restrict cond, jaeger_mutex* restrict mutex);

int jaeger_do_once(jaeger_once* once, void (*init_routine)(void));

void jaeger_thread_local_destroy(jaeger_thread_local* local);

bool jaeger_thread_local_init(jaeger_thread_local* local);

jaeger_destructible* jaeger_thread_local_get_value(jaeger_thread_local* local);

bool jaeger_thread_local_set_value(jaeger_thread_local* local,
                                   jaeger_destructible* value);

/** Implements deadlock avoidance similar to `std::lock`.
 * Based on smart & polite algorithm in
 * http://howardhinnant.github.io/dining_philosophers.html.
 */
void jaeger_lock(jaeger_mutex* restrict lock0, jaeger_mutex* restrict lock1);

#ifdef __cplusplus
} /* extern C */
#endif /* __cplusplus */

#endif /* JAEGERTRACINGC_THREADING_H */
