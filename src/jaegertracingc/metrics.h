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

#define JAEGERTRACINGC_COUNTER_SUBCLASS     \
    JAEGERTRACINGC_DESTRUCTIBLE_SUBCLASS;   \
    /**                                     \
     * Increment count.                     \
     * @param counter Counter to increment. \
     * @param delta Amount to increment.    \
     */                                     \
    void (*inc)(struct jaeger_counter * counter, int64_t delta);

/**
 * Counter metric interface. Counters are used to keep track of a total
 * number of events (i.e. number of spans reported).
 */
typedef struct jaeger_counter {
    JAEGERTRACINGC_COUNTER_SUBCLASS;
} jaeger_counter;

/**
 * Implements the counter interface with basic 64 bit integer value.
 */
typedef struct jaeger_default_counter {
    JAEGERTRACINGC_COUNTER_SUBCLASS;
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

#define JAEGERTRACINGC_GAUGE_SUBCLASS     \
    JAEGERTRACINGC_DESTRUCTIBLE_SUBCLASS; \
    /**                                   \
     * Update current value of gauge.     \
     * @param gauge Gauge to update.      \
     * @param amount New amount.          \
     */                                   \
    void (*update)(struct jaeger_gauge * gauge, int64_t amount);

/**
 * Gauge metric interface. Gauges are used to keep track of a changing number
 * (i.e. number of items currently in a queue).
 */
typedef struct jaeger_gauge {
    JAEGERTRACINGC_GAUGE_SUBCLASS;
} jaeger_gauge;

/**
 * Implements gauge interface with basic 64 bit integer value.
 */
typedef struct jaeger_default_gauge {
    JAEGERTRACINGC_GAUGE_SUBCLASS;
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

static inline void jaeger_metrics_destroy(jaeger_metrics* metrics)
{
    if (metrics == NULL) {
        return;
    }

#define JAEGERTRACINGC_METRICS_DESTROY_IF_NOT_NULL(member)                    \
    do {                                                                      \
        if (metrics->member != NULL) {                                        \
            metrics->member->destroy((jaeger_destructible*) metrics->member); \
            jaeger_free(metrics->member);                                     \
            metrics->member = NULL;                                           \
        }                                                                     \
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

/**
 * Initialize a new metrics container with default metric implementations.
 * @param metrics Metrics instance to initialize.
 * @return True on success, false otherwise.
 */
static inline bool jaeger_default_metrics_init(jaeger_metrics* metrics)
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

/* Shared instance of null metrics. DO NOT MODIFY MEMBERS! */
jaeger_metrics* jaeger_null_metrics();

#ifdef __cplusplus
} /* extern C */
#endif /* __cplusplus */

#endif /* JAEGERTRACINGC_METRICS_H */
