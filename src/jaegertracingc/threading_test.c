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
#include "jaegertracingc/clock.h"
#include "jaegertracingc/logging.h"
#include "unity.h"

static void* lock_func(void* arg)
{
    TEST_ASSERT_NOT_NULL(arg);
    jaeger_mutex* locks = arg;
    jaeger_lock(&locks[0], &locks[1]);
    jaeger_mutex_unlock(&locks[0]);
    jaeger_mutex_unlock(&locks[1]);
    return NULL;
}

void test_threading()
{
    srand(time(NULL));

    jaeger_mutex locks[2] = {JAEGERTRACINGC_MUTEX_INIT,
                             JAEGERTRACINGC_MUTEX_INIT};

    jaeger_lock(&locks[0], &locks[1]);

    jaeger_thread thread;
    jaeger_thread_init(&thread, &lock_func, locks);

    jaeger_mutex_unlock(&locks[0]);
    jaeger_duration sleep_duration = {
        .value = {.tv_sec = 0,
                  .tv_nsec = 0.01 * JAEGERTRACINGC_NANOSECONDS_PER_SECOND}};
    nanosleep(&sleep_duration.value, NULL);

    jaeger_mutex_unlock(&locks[1]);
    sleep_duration.value.tv_nsec = 0.1 * JAEGERTRACINGC_NANOSECONDS_PER_SECOND;
    nanosleep(&sleep_duration.value, NULL);

    jaeger_thread_join(thread, NULL);
    jaeger_mutex_destroy(&locks[0]);
    jaeger_mutex_destroy(&locks[1]);
}
