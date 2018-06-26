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

#ifndef JAEGERTRACINGC_TOKEN_BUCKET_H
#define JAEGERTRACINGC_TOKEN_BUCKET_H

/**
 * @file
 * Token bucket implementation.
 * @see jaeger_rate_limiting_sampler
 */

#include "jaegertracingc/clock.h"
#include "jaegertracingc/common.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct jaeger_token_bucket {
    double credits_per_second;
    double max_balance;
    double balance;
    jaeger_duration last_tick;
} jaeger_token_bucket;

void jaeger_token_bucket_init(jaeger_token_bucket* tok,
                              double credits_per_second,
                              double max_balance);

bool jaeger_token_bucket_check_credit(jaeger_token_bucket* tok, double cost);

#ifdef __cplusplus
} /* extern C */
#endif /* __cplusplus */

#endif /* JAEGERTRACINGC_TOKEN_BUCKET_H */
