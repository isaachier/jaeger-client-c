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
#include "jaegertracingc/alloc.h"

typedef struct default_counter {
    JAEGERTRACINGC_COUNTER_SUBCLASS;
    int64_t total;
} default_counter;

static void default_counter_inc(
    jaeger_counter* counter, int64_t delta)
{
    assert(counter != NULL);
    default_counter* c = (default_counter*) counter;
    c->total += delta;
}

jaeger_counter* jaeger_default_counter_init()
{
    jaeger_counter* counter =
        jaeger_default_alloc.malloc(
            &jaeger_default_alloc, sizeof(default_counter));
    if (counter == NULL) {
        fprintf(stderr, "Cannot allocate default counter\n");
        return NULL;
    }
    default_counter* c = (default_counter*) counter;
    c->total = 0;
    c->inc = &default_counter_inc;
    return counter;
}

static void null_counter_inc(jaeger_counter* counter, int64_t delta)
{
    (void) counter;
    (void) delta;
}

jaeger_counter* jaeger_null_counter_init()
{
    jaeger_counter* counter =
        jaeger_default_alloc.malloc(
            &jaeger_default_alloc, sizeof(jaeger_counter));
    if (counter == NULL) {
        fprintf(stderr, "Cannot allocate null counter\n");
        return NULL;
    }
    counter->inc = &null_counter_inc;
    return counter;
}

typedef struct default_gauge {
    JAEGERTRACINGC_GAUGE_SUBCLASS;
    int64_t amount;
} default_gauge;

static void default_gauge_update(jaeger_gauge* gauge, int64_t amount)
{
    assert(gauge != NULL);
    default_gauge* g = (default_gauge*) gauge;
    g->amount = amount;
}

jaeger_gauge* jaeger_default_gauge_init()
{
    jaeger_gauge* gauge =
        jaeger_default_alloc.malloc(
            &jaeger_default_alloc, sizeof(default_gauge));
    if (gauge == NULL) {
        fprintf(stderr, "Cannot allocate default gauge\n");
        return NULL;
    }
    default_gauge* g = (default_gauge*) gauge;
    g->amount = 0;
    g->update = &default_gauge_update;
}

static void null_gauge_update(jaeger_gauge* gauge, int64_t amount)
{
    (void) gauge;
    (void) amount;
}

jaeger_gauge* jaeger_null_gauge_init()
{
    jaeger_gauge* gauge =
        jaeger_default_alloc.malloc(
            &jaeger_default_alloc, sizeof(jaeger_gauge));
    if (gauge == NULL) {
        fprintf(stderr, "Cannot allocate null gauge\n");
        return NULL;
    }
    gauge->update = &null_gauge_update;
    return gauge;
}
