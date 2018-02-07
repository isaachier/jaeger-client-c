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
#include <stdio.h>

static void noop_destroy(jaeger_destructible* destructible)
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
    counter->destroy = &noop_destroy;
    counter->inc = &jaeger_default_counter_inc;
    return true;
}

static void jaeger_null_counter_inc(jaeger_counter* counter, int64_t delta)
{
    (void) counter;
    (void) delta;
}

bool jaeger_null_counter_init(jaeger_counter* counter)
{
    assert(counter != NULL);
    counter->destroy = &noop_destroy;
    counter->inc = &jaeger_null_counter_inc;
    return true;
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
    gauge->destroy = &noop_destroy;
    gauge->update = &jaeger_default_gauge_update;
    gauge->amount = 0;
    return true;
}

static void jaeger_null_gauge_update(jaeger_gauge* gauge, int64_t amount)
{
    (void) gauge;
    (void) amount;
}

bool jaeger_null_gauge_init(jaeger_gauge* gauge)
{
    assert(gauge != NULL);
    gauge->destroy = &noop_destroy;
    gauge->update = &jaeger_null_gauge_update;
    return true;
}
