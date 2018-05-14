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

static inline int jaeger_thread_init(jaeger_thread* thread,
                                     void* (*start_routine)(void*),
                                     void* arg)
{
    return pthread_create(thread, NULL, start_routine, arg);
}

static inline int jaeger_thread_join(jaeger_thread thread, void** return_value)
{
    return pthread_join(thread, return_value);
}

static inline int jaeger_yield()
{
    return sched_yield();
}

typedef pthread_mutex_t jaeger_mutex;

#define JAEGERTRACINGC_MUTEX_INIT PTHREAD_MUTEX_INITIALIZER

static inline int jaeger_mutex_lock(jaeger_mutex* mutex)
{
    return pthread_mutex_lock(mutex);
}

static inline int jaeger_mutex_try_lock(jaeger_mutex* mutex)
{
    return pthread_mutex_trylock(mutex);
}

static inline int jaeger_mutex_unlock(jaeger_mutex* mutex)
{
    return pthread_mutex_unlock(mutex);
}

static inline int jaeger_mutex_destroy(jaeger_mutex* mutex)
{
    return pthread_mutex_destroy(mutex);
}

typedef pthread_cond_t jaeger_cond;

#define JAEGERTRACINGC_COND_INIT PTHREAD_COND_INITIALIZER

static inline int jaeger_cond_destroy(jaeger_cond* cond)
{
    return pthread_cond_destroy(cond);
}

static inline int jaeger_cond_signal(jaeger_cond* cond)
{
    return pthread_cond_signal(cond);
}

static inline int jaeger_cond_wait(jaeger_cond* restrict cond,
                                   jaeger_mutex* restrict mutex)
{
    return pthread_cond_wait(cond, mutex);
}

typedef pthread_once_t jaeger_once;
#define JAEGERTRACINGC_ONCE_INIT PTHREAD_ONCE_INIT

static inline int jaeger_do_once(jaeger_once* once, void (*init_routine)(void))
{
    return pthread_once(once, init_routine);
}

typedef struct jaeger_thread_local {
    pthread_key_t key;
    bool initialized;
} jaeger_thread_local;

static inline void jaeger_thread_local_destroy(jaeger_thread_local* local)
{
    if (local == NULL) {
        return;
    }
    if (!local->initialized) {
        return;
    }
    jaeger_destructible_destroy(
        (jaeger_destructible*) pthread_getspecific(local->key));
    pthread_key_delete(local->key);
    local->initialized = false;
}

static inline bool jaeger_thread_local_init(jaeger_thread_local* local)
{
    assert(local != NULL);
    local->initialized = false;
    const int return_code =
        pthread_key_create(&local->key, &jaeger_destructible_destroy_wrapper);
    if (return_code != 0) {
        jaeger_log_error("Cannot create jaeger_thread_local, return code = "
                         "%d, errno = %d",
                         return_code,
                         errno);
        return false;
    }

    local->initialized = true;
    return true;
}

static inline bool
jaeger_thread_local_check_initialized(jaeger_thread_local* local)
{
    if (!local->initialized) {
        jaeger_log_error("Tried to access uninitialized thread local");
    }
    return local->initialized;
}

static inline jaeger_destructible*
jaeger_thread_local_get_value(jaeger_thread_local* local)
{
    assert(local != NULL);
    if (!jaeger_thread_local_check_initialized(local)) {
        return NULL;
    }
    return (jaeger_destructible*) pthread_getspecific(local->key);
}

static inline bool jaeger_thread_local_set_value(jaeger_thread_local* local,
                                                 jaeger_destructible* value)
{
    assert(local != NULL);
    if (!jaeger_thread_local_check_initialized(local)) {
        return false;
    }
    /* Destroy the current value. jaeger_destructible_destroy handles NULL, so
     * not checking here. */
    jaeger_destructible_destroy(
        (jaeger_destructible*) pthread_getspecific(local->key));
    const int return_code = pthread_setspecific(local->key, (void*) value);
    if (return_code != 0) {
        jaeger_log_error("Cannot create jaeger_thread_local, return code = "
                         "%d, errno = %d",
                         return_code,
                         errno);
    }
    return (return_code == 0);
}

#else

typedef struct {
    void* return_value;
} jaeger_thread;

static inline int jaeger_thread_init(jaeger_thread* thread,
                                     void* (*start_routine)(void*),
                                     void* arg)
{
    assert(thread != NULL);
    /* Run code synchronously in single-threaded environment. */
    thread->return_value = start_routine(arg);
    return 0;
}

static inline int jaeger_thread_join(jaeger_thread thread, void** return_value)
{
    (void) thread;
    (void) return_value;
    return 0;
}

static inline int jaeger_yield()
{
    return 0;
}

typedef struct {
    bool locked;
} jaeger_mutex;

#define JAEGERTRACINGC_MUTEX_INIT \
    {                             \
        .locked = false           \
    }

static inline int jaeger_mutex_lock(jaeger_mutex* mutex)
{
    assert(mutex != NULL);
    assert(!mutex->locked);
    mutex->locked = true;
    return 0;
}

static inline int jaeger_mutex_try_lock(jaeger_mutex* mutex)
{
    assert(mutex != NULL);
    if (!mutex->locked) {
        mutex->locked = true;
        return 0;
    }
    return EBUSY;
}

static inline int jaeger_mutex_unlock(jaeger_mutex* mutex)
{
    assert(mutex != NULL);
    assert(mutex->locked);
    mutex->locked = false;
    return 0;
}

static inline int jaeger_mutex_destroy(jaeger_mutex* mutex)
{
    if (mutex->locked) {
        mutex->locked = false;
        return EBUSY;
    }
    return 0;
}

typedef struct {
    bool signal;
} jaeger_cond;

#define JAEGERTRACINGC_COND_INIT \
    {                            \
        .signal = false          \
    }

static inline int jaeger_cond_destroy(jaeger_cond* cond)
{
    assert(cond != NULL);
    cond->signal = false;
    return 0;
}

static inline int jaeger_cond_signal(jaeger_cond* cond)
{
    assert(cond != NULL);
    cond->signal = true;
    return 0;
}

static inline int jaeger_cond_wait(jaeger_cond* restrict cond,
                                   jaeger_mutex* restrict mutex)
{
    assert(mutex->locked);
    mutex->locked = false;
    while (!cond->signal)
        ;
    mutex->locked = true;
}

typedef struct {
    bool has_run;
} jaeger_once;

#define JAEGERTRACINGC_ONCE_INIT \
    {                            \
        .has_run = false         \
    }

static inline int jaeger_do_once(jaeger_once* once, void (*init_routine)(void))
{
    assert(once != NULL);
    assert(init_routine != NULL);
    if (!once->has_run) {
        once->has_run = true;
        init_routine();
    }
    return 0;
}

typedef struct jaeger_thread_local {
    jaeger_destructible* value;
} jaeger_thread_local;

static inline void jaeger_thread_local_destroy(jaeger_thread_local* local)
{
    if (local == NULL) {
        return;
    }
    jaeger_destructible_destroy(local->value);
}

static inline bool jaeger_thread_local_init(jaeger_thread_local* local)
{
    assert(local != NULL);
    local->value = NULL;
    return 0;
}

static inline jaeger_destructible*
jaeger_thread_local_get_value(jaeger_thread_local* local)
{
    assert(local != NULL);
    return local->value;
}

static inline bool jaeger_thread_local_set_value(jaeger_thread_local* local,
                                                 jaeger_destructible* value)
{
    assert(local != NULL);
    jaeger_destructible_destroy(local->value);
    local->value = value;
    return local->value;
}

#endif /* JAEGERTRACINGC_MT */

/* Implements deadlock avoidance similar to `std::lock`.
 * Based on smart & polite algorithm in
 * http://howardhinnant.github.io/dining_philosophers.html. */
static inline void jaeger_lock(jaeger_mutex* lock0, jaeger_mutex* lock1)
{
    while (true) {
        /* Lock lock0, then try locking lock1. */
        jaeger_mutex_lock(lock0);
        if (jaeger_mutex_try_lock(lock1) == 0) {
            break;
        }
        jaeger_mutex_unlock(lock0);

        /* Be polite. */
        jaeger_yield();

        /* Originally written as a separate block attempting to lock lock1
         * first, then attempting jaeger_mutex_try_lock on lock0. However, this
         * swapping trick allows the unit test to hit 100% coverage without
         * calling jaeger_yield more than once.
         */
        jaeger_mutex* tmp = lock0;
        lock0 = lock1;
        lock1 = tmp;
    }
}

#ifdef __cplusplus
} /* extern C */
#endif /* __cplusplus */

#endif /* JAEGERTRACINGC_THREADING_H */
