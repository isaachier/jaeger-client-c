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

#include "jaegertracingc/ticker.h"

#include <stdio.h>

static void* jaeger_ticker_run(void* context)
{
    assert(context != NULL);
    jaeger_ticker* ticker = (jaeger_ticker*) context;
    while (true) {
        pthread_mutex_lock(&ticker->mutex);
        bool running = ticker->running;
        pthread_mutex_unlock(&ticker->mutex);
        if (!running) {
            return NULL;
        }

        ticker->func(ticker->context);

        pthread_mutex_lock(&ticker->mutex);
        const jaeger_duration interval = ticker->interval;
        jaeger_duration start_time;
        clock_gettime(CLOCK_MONOTONIC, &start_time);
        jaeger_duration end_time = start_time;
        jaeger_duration wait_interval = {0, 0};
        while (running && (wait_interval.tv_sec < interval.tv_sec ||
                           (wait_interval.tv_sec == interval.tv_sec &&
                            wait_interval.tv_nsec < interval.tv_sec))) {
            pthread_cond_timedwait(&ticker->cond, &ticker->mutex, &interval);
            clock_gettime(CLOCK_MONOTONIC, &end_time);
            const bool result = jaeger_duration_subtract(
                &end_time, &start_time, &wait_interval);
            (void) result;
            assert(result);
        }
        running = ticker->running;
        pthread_mutex_unlock(&ticker->mutex);
        if (!running) {
            return NULL;
        }
    }
}

int jaeger_ticker_start(jaeger_ticker* ticker,
                        const jaeger_duration* interval,
                        void (*func)(void*),
                        void* context)
{
    assert(ticker != NULL);
    assert(interval != NULL);
    assert(func != NULL);
    assert(context != NULL);
    ticker->interval = *interval;
    ticker->func = func;
    ticker->context = context;
    ticker->running = true;
    return pthread_create(&ticker->thread, NULL, &jaeger_ticker_run, ticker);
}
