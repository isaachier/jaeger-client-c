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

#include "jaegertracingc/metrics.h"
#include "jaegertracingc/threading.h"

static void null_destroy(jaeger_destructible* destructible)
{
    (void) destructible;
}

static void jaeger_default_counter_inc(jaeger_counter* counter, int64_t delta)
{
    assert(counter != NULL);
    jaeger_default_counter* c = (jaeger_default_counter*) counter;
#ifdef JAEGERTRACINGC_HAVE_ATOMICS
    __atomic_add_fetch(&c->total, delta, __ATOMIC_RELAXED);
#else
    jaeger_mutex_lock(&c->mutex);
    c->total += delta;
    jaeger_mutex_unlock(&c->mutex);
#endif /* JAEGERTRACINGC_HAVE_ATOMICS */
}

void jaeger_default_counter_init(jaeger_default_counter* counter)
{
    assert(counter != NULL);
    counter->total = 0;
    ((jaeger_destructible*) counter)->destroy = &null_destroy;
    ((jaeger_counter*) counter)->inc = &jaeger_default_counter_inc;
}

static void null_counter_inc(jaeger_counter* counter, int64_t delta)
{
    (void) counter;
    (void) delta;
}

static jaeger_counter null_counter;

static void init_null_counter()
{
    null_counter = (jaeger_counter){.base = {.destroy = &null_destroy},
                                    .inc = &null_counter_inc};
}

jaeger_counter* jaeger_null_counter()
{
    static jaeger_once once = JAEGERTRACINGC_ONCE_INIT;
    jaeger_do_once(&once, &init_null_counter);
    return &null_counter;
}

static void jaeger_default_gauge_update(jaeger_gauge* gauge, int64_t amount)
{
    assert(gauge != NULL);
    jaeger_default_gauge* g = (jaeger_default_gauge*) gauge;
#ifdef JAEGERTRACINGC_HAVE_ATOMICS
    __atomic_store_n(&g->amount, amount, __ATOMIC_RELAXED);
#else
    jaeger_mutex_lock(&g->mutex);
    g->amount = amount;
    jaeger_mutex_unlock(&g->mutex);
#endif /* JAEGERTRACINGC_HAVE_ATOMICS */
}

void jaeger_default_gauge_init(jaeger_default_gauge* gauge)
{
    assert(gauge != NULL);
    ((jaeger_destructible*) gauge)->destroy = &null_destroy;
    ((jaeger_gauge*) gauge)->update = &jaeger_default_gauge_update;
    gauge->amount = 0;
}

static void null_gauge_update(jaeger_gauge* gauge, int64_t amount)
{
    (void) gauge;
    (void) amount;
}

static jaeger_gauge null_gauge;

static void init_null_gauge()
{
    null_gauge = (jaeger_gauge){.base = {.destroy = null_destroy},
                                .update = null_gauge_update};
}

jaeger_gauge* jaeger_null_gauge()
{
    static jaeger_once once = JAEGERTRACINGC_ONCE_INIT;
    jaeger_do_once(&once, &init_null_gauge);
    return &null_gauge;
}

static jaeger_metrics null_metrics;

static void init_null_metrics()
{
    null_metrics = (jaeger_metrics){
#define SET_COUNTERS(member) .member = jaeger_null_counter(),
        JAEGERTRACINGC_METRICS_COUNTERS(SET_COUNTERS)
#define SET_GAUGES(member) .member = jaeger_null_gauge(),
            JAEGERTRACINGC_METRICS_GAUGES(SET_GAUGES)};
}

jaeger_metrics* jaeger_null_metrics()
{
    static jaeger_once once = JAEGERTRACINGC_ONCE_INIT;
    jaeger_do_once(&once, &init_null_metrics);
    return &null_metrics;
}

void jaeger_metrics_destroy(jaeger_metrics* metrics)
{
    if (metrics == NULL) {
        return;
    }

#define JAEGERTRACINGC_METRICS_DESTROY_IF_NOT_NULL(member)                   \
    do {                                                                     \
        if (metrics->member != NULL) {                                       \
            jaeger_destructible* d = (jaeger_destructible*) metrics->member; \
            d->destroy(d);                                                   \
            jaeger_free(metrics->member);                                    \
            metrics->member = NULL;                                          \
        }                                                                    \
    } while (0);

    JAEGERTRACINGC_METRICS_COUNTERS(JAEGERTRACINGC_METRICS_DESTROY_IF_NOT_NULL)
    JAEGERTRACINGC_METRICS_GAUGES(JAEGERTRACINGC_METRICS_DESTROY_IF_NOT_NULL)

#undef JAEGERTRACINGC_METRICS_DESTROY_IF_NOT_NULL
}

#define JAEGERTRACINGC_METRICS_ALLOC_INIT(member, base_type, type, init)     \
    do {                                                                     \
        if (success) {                                                       \
            metrics->member = (base_type*) jaeger_malloc(sizeof(type));      \
            if (metrics->member == NULL) {                                   \
                jaeger_log_error("Cannot allocate metrics member " #member); \
                success = false;                                             \
            }                                                                \
            else {                                                           \
                init((type*) metrics->member);                               \
            }                                                                \
        }                                                                    \
    } while (0)

#define JAEGERTRACINGC_METRICS_INIT_IMPL(counter_init, gauge_init) \
    do {                                                           \
        assert(metrics != NULL);                                   \
        memset(metrics, 0, sizeof(*metrics));                      \
        bool success = true;                                       \
                                                                   \
        JAEGERTRACINGC_METRICS_COUNTERS(counter_init)              \
        JAEGERTRACINGC_METRICS_GAUGES(gauge_init)                  \
                                                                   \
        if (!success) {                                            \
            jaeger_metrics_destroy(metrics);                       \
        }                                                          \
                                                                   \
        return success;                                            \
    } while (0)

bool jaeger_default_metrics_init(jaeger_metrics* metrics)
{
#define JAEGERTRACINGC_DEFAULT_COUNTER_ALLOC_INIT(member)     \
    JAEGERTRACINGC_METRICS_ALLOC_INIT(member,                 \
                                      jaeger_counter,         \
                                      jaeger_default_counter, \
                                      jaeger_default_counter_init);
#define JAEGERTRACINGC_DEFAULT_GAUGE_ALLOC_INIT(member)     \
    JAEGERTRACINGC_METRICS_ALLOC_INIT(member,               \
                                      jaeger_gauge,         \
                                      jaeger_default_gauge, \
                                      jaeger_default_gauge_init);

    JAEGERTRACINGC_METRICS_INIT_IMPL(JAEGERTRACINGC_DEFAULT_COUNTER_ALLOC_INIT,
                                     JAEGERTRACINGC_DEFAULT_GAUGE_ALLOC_INIT);

#undef JAEGERTRACINGC_DEFAULT_COUNTER_ALLOC_INIT
#undef JAEGERTRACINGC_DEFAULT_GAUGE_ALLOC_INIT
}
