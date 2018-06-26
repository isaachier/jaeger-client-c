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

#include "jaegertracingc/threading.h"

#ifdef JAEGERTRACINGC_MT

int jaeger_thread_init(jaeger_thread* thread,
                       void* (*start_routine)(void*),
                       void* arg)
{
    return pthread_create(thread, NULL, start_routine, arg);
}

int jaeger_thread_join(jaeger_thread thread, void** return_value)
{
    return pthread_join(thread, return_value);
}

int jaeger_yield(void)
{
    return sched_yield();
}

int jaeger_mutex_lock(jaeger_mutex* mutex)
{
    return pthread_mutex_lock(mutex);
}

int jaeger_mutex_try_lock(jaeger_mutex* mutex)
{
    return pthread_mutex_trylock(mutex);
}

int jaeger_mutex_unlock(jaeger_mutex* mutex)
{
    return pthread_mutex_unlock(mutex);
}

int jaeger_mutex_destroy(jaeger_mutex* mutex)
{
    return pthread_mutex_destroy(mutex);
}

int jaeger_cond_destroy(jaeger_cond* cond)
{
    return pthread_cond_destroy(cond);
}

int jaeger_cond_signal(jaeger_cond* cond)
{
    return pthread_cond_signal(cond);
}

int jaeger_cond_wait(jaeger_cond* restrict cond, jaeger_mutex* restrict mutex)
{
    return pthread_cond_wait(cond, mutex);
}

int jaeger_do_once(jaeger_once* once, void (*init_routine)(void))
{
    return pthread_once(once, init_routine);
}

void jaeger_thread_local_destroy(jaeger_thread_local* local)
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

bool jaeger_thread_local_init(jaeger_thread_local* local)
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

bool jaeger_thread_local_check_initialized(jaeger_thread_local* local)
{
    if (!local->initialized) {
        jaeger_log_error("Tried to access uninitialized thread local");
    }
    return local->initialized;
}

jaeger_destructible* jaeger_thread_local_get_value(jaeger_thread_local* local)
{
    assert(local != NULL);
    if (!jaeger_thread_local_check_initialized(local)) {
        return NULL;
    }
    return (jaeger_destructible*) pthread_getspecific(local->key);
}

bool jaeger_thread_local_set_value(jaeger_thread_local* local,
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

int jaeger_thread_init(jaeger_thread* thread,
                       void* (*start_routine)(void*),
                       void* arg)
{
    assert(thread != NULL);
    /* Run code synchronously in single-threaded environment. */
    thread->return_value = start_routine(arg);
    return 0;
}

int jaeger_thread_join(jaeger_thread thread, void** return_value)
{
    (void) thread;
    (void) return_value;
    return 0;
}

int jaeger_yield(void)
{
    return 0;
}

int jaeger_mutex_lock(jaeger_mutex* mutex)
{
    assert(mutex != NULL);
    assert(!mutex->locked);
    mutex->locked = true;
    return 0;
}

int jaeger_mutex_try_lock(jaeger_mutex* mutex)
{
    assert(mutex != NULL);
    if (!mutex->locked) {
        mutex->locked = true;
        return 0;
    }
    return EBUSY;
}

int jaeger_mutex_unlock(jaeger_mutex* mutex)
{
    assert(mutex != NULL);
    assert(mutex->locked);
    mutex->locked = false;
    return 0;
}

int jaeger_mutex_destroy(jaeger_mutex* mutex)
{
    if (mutex->locked) {
        mutex->locked = false;
        return EBUSY;
    }
    return 0;
}

int jaeger_cond_destroy(jaeger_cond* cond)
{
    assert(cond != NULL);
    cond->signal = false;
    return 0;
}

int jaeger_cond_signal(jaeger_cond* cond)
{
    assert(cond != NULL);
    cond->signal = true;
    return 0;
}

int jaeger_cond_wait(jaeger_cond* restrict cond, jaeger_mutex* restrict mutex)
{
    assert(mutex->locked);
    mutex->locked = false;
    while (!cond->signal)
        ;
    mutex->locked = true;
}

int jaeger_do_once(jaeger_once* once, void (*init_routine)(void))
{
    assert(once != NULL);
    assert(init_routine != NULL);
    if (!once->has_run) {
        once->has_run = true;
        init_routine();
    }
    return 0;
}

void jaeger_thread_local_destroy(jaeger_thread_local* local)
{
    if (local == NULL) {
        return;
    }
    jaeger_destructible_destroy(local->value);
}

bool jaeger_thread_local_init(jaeger_thread_local* local)
{
    assert(local != NULL);
    local->value = NULL;
    return 0;
}

jaeger_destructible* jaeger_thread_local_get_value(jaeger_thread_local* local)
{
    assert(local != NULL);
    return local->value;
}

bool jaeger_thread_local_set_value(jaeger_thread_local* local,
                                   jaeger_destructible* value)
{
    assert(local != NULL);
    jaeger_destructible_destroy(local->value);
    local->value = value;
    return local->value;
}

#endif /* JAEGERTRACINGC_MT */

void jaeger_lock(jaeger_mutex* restrict lock0, jaeger_mutex* restrict lock1)
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
