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

#ifndef JAEGERTRACINGC_UNIT_TEST_DRIVER_H
#define JAEGERTRACINGC_UNIT_TEST_DRIVER_H

#include "alloc_test.h"
#include "duration_test.h"
#include "metrics_test.h"
#include "net_test.h"
#include "sampler_test.h"
#include "tag_test.h"
#include "token_bucket_test.h"
#include "unity.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

static inline void run_tests()
{
    RUN_TEST(test_alloc);
    RUN_TEST(test_duration);
    RUN_TEST(test_metrics);
    RUN_TEST(test_net);
    RUN_TEST(test_const_sampler);
    RUN_TEST(test_probabilistic_sampler);
    RUN_TEST(test_rate_limiting_sampler);
    RUN_TEST(test_guaranteed_throughput_probabilistic_sampler);
    RUN_TEST(test_adaptive_sampler);
    RUN_TEST(test_remotely_controlled_sampler);
    RUN_TEST(test_tag);
    RUN_TEST(test_token_bucket);
}

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* JAEGERTRACINGC_UNIT_TEST_DRIVER_H */
