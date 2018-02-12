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

#include "jaegertracingc/alloc.h"
#include "jaegertracingc/logging.h"
#include "jaegertracingc/metrics.h"
#include "unity.h"

void test_metrics()
{
    jaeger_default_counter default_counter;
    jaeger_default_counter_init(&default_counter);
    default_counter.inc((jaeger_counter*) &default_counter, 2);
    TEST_ASSERT_EQUAL(2, default_counter.total);

    jaeger_counter null_counter;
    jaeger_null_counter_init(&null_counter);
    null_counter.inc(&null_counter, -1);

    jaeger_default_gauge default_gauge;
    jaeger_default_gauge_init(&default_gauge);
    default_gauge.update((jaeger_gauge*) &default_gauge, 3);
    TEST_ASSERT_EQUAL(3, default_gauge.amount);

    jaeger_gauge null_gauge;
    jaeger_null_gauge_init(&null_gauge);
    null_gauge.update(&null_gauge, 4);

    jaeger_logger* logger = jaeger_null_logger();
    jaeger_metrics metrics;
    jaeger_default_metrics_init(&metrics, logger);
    jaeger_metrics_destroy(&metrics);
    jaeger_null_metrics_init(&metrics, logger);
    jaeger_metrics_destroy(&metrics);
}
