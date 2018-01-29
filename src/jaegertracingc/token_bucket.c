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

#include "jaegertracingc/token_bucket.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>

#include "jaegertracingc/time.h"

#define NS_PER_S JAEGERTRACINGC_NANOSECONDS_PER_SECOND

#define MIN(a, b) (((b) < (a)) ? (b) : (a))

struct jaeger_token_bucket {
    double credits_per_second;
    double max_balance;
    double balance;
    jaeger_duration last_tick;
};

jaeger_token_bucket* jaeger_token_bucket_init(
    const jaeger_alloc* alloc, double credits_per_second, double max_balance)
{
    assert(alloc != NULL);
    jaeger_token_bucket* tok = alloc->malloc(sizeof(jaeger_token_bucket));
    if (tok == NULL) {
        fprintf(stderr, "ERROR: Cannot allocate token bucket\n");
        return NULL;
    }
    tok->credits_per_second = credits_per_second;
    tok->max_balance = max_balance;
    jaeger_duration_now(&tok->last_tick);
    return tok;
}

int jaeger_token_bucket_check_credit(jaeger_token_bucket* tok, double cost)
{
    assert(tok != NULL);
    jaeger_duration current_time;
    jaeger_duration_now(&current_time);
    jaeger_duration interval;
    const int result =
        jaeger_duration_subtract(&current_time, &tok->last_tick, &interval);
    (void) result;
    assert(result == 0);
    const double diff =
        (((double) interval.tv_nsec) / NS_PER_S + interval.tv_sec) *
        tok->credits_per_second;
    tok->balance = MIN(tok->max_balance, tok->balance + diff);
    tok->last_tick = current_time;
    return 0;
}
