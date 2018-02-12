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
#include <assert.h>
#include "jaegertracingc/threading.h"

static void null_destroy(jaeger_destructible* destructible)
{
    (void) destructible;
}

static void jaeger_default_counter_inc(jaeger_counter* counter, int64_t delta)
{
    assert(counter != NULL);
    jaeger_default_counter* c = (jaeger_default_counter*) counter;
    c->total += delta;
}

bool jaeger_default_counter_init(jaeger_default_counter* counter)
{
    assert(counter != NULL);
    counter->total = 0;
    counter->destroy = &null_destroy;
    counter->inc = &jaeger_default_counter_inc;
    return true;
}

static void null_counter_inc(jaeger_counter* counter, int64_t delta)
{
    (void) counter;
    (void) delta;
}

static jaeger_counter null_counter;

static void init_null_counter()
{
    null_counter =
        (jaeger_counter){.destroy = &null_destroy, .inc = &null_counter_inc};
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
    g->amount = amount;
}

bool jaeger_default_gauge_init(jaeger_default_gauge* gauge)
{
    assert(gauge != NULL);
    gauge->destroy = &null_destroy;
    gauge->update = &jaeger_default_gauge_update;
    gauge->amount = 0;
    return true;
}

static void null_gauge_update(jaeger_gauge* gauge, int64_t amount)
{
    (void) gauge;
    (void) amount;
}

static jaeger_gauge null_gauge;

static void init_null_gauge()
{
    null_gauge =
        (jaeger_gauge){.destroy = null_destroy, .update = null_gauge_update};
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
