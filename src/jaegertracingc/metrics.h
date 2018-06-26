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
 * Metrics interfaces to track various numbers while tracing.
 */

#ifndef JAEGERTRACINGC_METRICS_H
#define JAEGERTRACINGC_METRICS_H

#include "jaegertracingc/common.h"

#ifndef JAEGERTRACINGC_HAVE_ATOMICS
#include "jaegertracingc/threading.h"
#endif /* JAEGERTRACINGC_HAVE_ATOMICS */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * Counter metric interface. Counters are used to keep track of a total
 * number of events (i.e. number of spans reported).
 * @extends jaeger_destructible
 */
typedef struct jaeger_counter {
    /** Base class member. */
    jaeger_destructible base;

    /**
     * Increment count.
     * @param counter Counter to increment.
     * @param delta Amount to increment.
     */
    void (*inc)(struct jaeger_counter* counter, int64_t delta);
} jaeger_counter;

/**
 * Implements the counter interface with basic 64 bit integer value.
 */
typedef struct jaeger_default_counter {
    jaeger_counter base;
    /** Current total. */
    int64_t total;
#ifndef JAEGERTRACINGC_HAVE_ATOMICS
    /** Lock to avoid data races. */
    jaeger_mutex mutex;
#endif /* JAEGERTRACINGC_HAVE_ATOMICS */
} jaeger_default_counter;

/**
 * Shared instance of null counter. The null counter does not do anything in its
 * inc() function. DO NOT MODIFY MEMBERS!
 */
jaeger_counter* jaeger_null_counter();

/**
 * Initialize a default counter.
 * @param counter Counter to initialize.
 */
void jaeger_default_counter_init(jaeger_default_counter* counter);

/**
 * Gauge metric interface. Gauges are used to keep track of a changing number
 * (i.e. number of items currently in a queue).
 * @extends jaeger_destructible
 */
typedef struct jaeger_gauge {
    /** Base class member. */
    jaeger_destructible base;

    /**
     * Update current value of gauge.
     * @param gauge Gauge to update.
     * @param amount New amount.
     */
    void (*update)(struct jaeger_gauge* gauge, int64_t amount);
} jaeger_gauge;

/**
 * Implements gauge interface with basic 64 bit integer value.
 */
typedef struct jaeger_default_gauge {
    jaeger_gauge base;
    /** Current amount. */
    int64_t amount;
#ifndef JAEGERTRACINGC_HAVE_ATOMICS
    /** Lock to avoid data races. */
    jaeger_mutex mutex;
#endif /* JAEGERTRACINGC_HAVE_ATOMICS */
} jaeger_default_gauge;

/**
 * Initialize a default gauge.
 * @param gauge Gauge object to be initialized. May not be NULL.
 */
void jaeger_default_gauge_init(jaeger_default_gauge* gauge);

/* Shared instance of null gauge. DO NOT MODIFY MEMBERS! */
jaeger_gauge* jaeger_null_gauge();

#define JAEGERTRACINGC_METRICS_COUNTERS(X) \
    X(traces_started_sampled)              \
    X(traces_started_not_sampled)          \
    X(traces_joined_sampled)               \
    X(traces_joined_not_sampled)           \
    X(spans_started)                       \
    X(spans_finished)                      \
    X(spans_sampled)                       \
    X(spans_not_sampled)                   \
    X(decoding_errors)                     \
    X(reporter_success)                    \
    X(reporter_failure)                    \
    X(reporter_dropped)                    \
    X(sampler_retrieved)                   \
    X(sampler_updated)                     \
    X(sampler_update_failure)              \
    X(sampler_query_failure)               \
    X(baggage_update_success)              \
    X(baggage_update_failure)              \
    X(baggage_truncate)                    \
    X(baggage_restrictions_update_success) \
    X(baggage_restrictions_update_failure)

#define JAEGERTRACINGC_METRICS_GAUGES(X) X(reporter_queue_length)

#define JAEGERTRACINGC_COUNTER_DECL(member) jaeger_counter* member;
#define JAEGERTRACINGC_GAUGE_DECL(member) jaeger_gauge* member;

typedef struct jaeger_metrics {
    JAEGERTRACINGC_METRICS_COUNTERS(JAEGERTRACINGC_COUNTER_DECL)
    JAEGERTRACINGC_METRICS_GAUGES(JAEGERTRACINGC_GAUGE_DECL)
} jaeger_metrics;

#undef JAEGERTRACINGC_COUNTER_DECL
#undef JAEGERTRACINGC_GAUGE_DECL

void jaeger_metrics_destroy(jaeger_metrics* metrics);

/**
 * Initialize a new metrics container with default metric implementations.
 * @param metrics Metrics instance to initialize.
 * @return True on success, false otherwise.
 */
bool jaeger_default_metrics_init(jaeger_metrics* metrics);

/* Shared instance of null metrics. DO NOT MODIFY MEMBERS! */
jaeger_metrics* jaeger_null_metrics();

#ifdef __cplusplus
} /* extern C */
#endif /* __cplusplus */

#endif /* JAEGERTRACINGC_METRICS_H */
