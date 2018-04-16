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

static inline void jaeger_token_bucket_init(jaeger_token_bucket* tok,
                                            double credits_per_second,
                                            double max_balance)
{
    assert(tok != NULL);
    tok->balance = max_balance;
    tok->credits_per_second = credits_per_second;
    tok->max_balance = max_balance;
    jaeger_duration_now(&tok->last_tick);
}

static inline bool jaeger_token_bucket_check_credit(jaeger_token_bucket* tok,
                                                    double cost)
{
    assert(tok != NULL);
    jaeger_duration current_time;
    jaeger_duration_now(&current_time);
    jaeger_duration interval;
    const bool result = jaeger_time_subtract(
        current_time.value, tok->last_tick.value, &interval.value);
    (void) result;
    assert(result);
    const double diff = (((double) interval.value.tv_nsec) /
                             JAEGERTRACINGC_NANOSECONDS_PER_SECOND +
                         interval.value.tv_sec) *
                        tok->credits_per_second;
    const double new_balance = tok->balance + diff;
    tok->balance =
        (tok->max_balance < new_balance) ? tok->max_balance : new_balance;
    tok->last_tick = current_time;
    if (tok->balance < cost) {
        return 0;
    }
    tok->balance -= cost;
    return 1;
}

#ifdef __cplusplus
} /* extern C */
#endif /* __cplusplus */

#endif /* JAEGERTRACINGC_TOKEN_BUCKET_H */
