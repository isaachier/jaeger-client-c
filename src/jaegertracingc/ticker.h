
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

#ifndef JAEGERTRACINGC_TICKER_H
#define JAEGERTRACINGC_TICKER_H

#include <pthread.h>
#include "jaegertracingc/common.h"
#include "jaegertracingc/duration.h"

typedef struct jaeger_ticker {
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    pthread_t thread;
    jaeger_duration interval;
    void (*func)(void*);
    bool running;
    void* context;
} jaeger_ticker;

#define JAEGERTRACINGC_TICKER_INIT                                            \
    {                                                                         \
        PTHREAD_MUTEX_INITIALIZER, PTHREAD_COND_INITIALIZER, 0, {0, 0}, NULL, \
            false, NULL                                                       \
    }

static inline void jaeger_ticker_init(jaeger_ticker* ticker)
{
    assert(ticker != NULL);
    *ticker = (jaeger_ticker) JAEGERTRACINGC_TICKER_INIT;
}

int jaeger_ticker_start(jaeger_ticker* ticker,
                        const jaeger_duration* interval,
                        void (*func)(void*),
                        void* context);

static inline void jaeger_ticker_stop(jaeger_ticker* ticker)
{
    assert(ticker != NULL);
    pthread_mutex_lock(&ticker->mutex);
    ticker->running = false;
    pthread_mutex_unlock(&ticker->mutex);
    pthread_cond_signal(&ticker->cond);
    pthread_join(ticker->thread, NULL);
}

#endif /* JAEGERTRACINGC_TICKER_H */
