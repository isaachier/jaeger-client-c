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

static void jaeger_default_counter_inc(jaeger_counter* counter, int64_t delta)
{
    assert(counter != NULL);
    jaeger_default_counter* c = (jaeger_default_counter*) counter;
    c->total += delta;
}

void jaeger_default_counter_init(jaeger_default_counter* counter)
{
    assert(counter != NULL);
    counter->total = 0;
    counter->inc = &jaeger_default_counter_inc;
}

static void jaeger_null_counter_inc(jaeger_counter* counter, int64_t delta)
{
    (void) counter;
    (void) delta;
}

void jaeger_null_counter_init(jaeger_counter* counter)
{
    assert(counter != NULL);
    counter->inc = &jaeger_null_counter_inc;
}

static void jaeger_default_gauge_update(jaeger_gauge* gauge, int64_t amount)
{
    assert(gauge != NULL);
    jaeger_default_gauge* g = (jaeger_default_gauge*) gauge;
    g->amount = amount;
}

void jaeger_default_gauge_init(jaeger_default_gauge* gauge)
{
    gauge->amount = 0;
    gauge->update = &jaeger_default_gauge_update;
}

static void jaeger_null_gauge_update(jaeger_gauge* gauge, int64_t amount)
{
    (void) gauge;
    (void) amount;
}

void jaeger_null_gauge_init(jaeger_gauge* gauge)
{
    gauge->update = &jaeger_null_gauge_update;
}
