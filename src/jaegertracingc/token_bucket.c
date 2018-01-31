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

#include "jaegertracingc/alloc.h"
#include "jaegertracingc/duration.h"

#define NS_PER_S JAEGERTRACINGC_NANOSECONDS_PER_SECOND

#define MIN(a, b) (((b) < (a)) ? (b) : (a))

struct jaeger_token_bucket {
    double credits_per_second;
    double max_balance;
    double balance;
    jaeger_duration last_tick;
#ifndef TESTING
    void (*time_fn)(jaeger_duration*);
#endif /* TESTING */
};

#ifdef TESTING
#define INIT_TIME(tok, current_time)                                           \
    do {                                                                       \
        jaeger_duration_now(&current_time);                                    \
    } while (0)
#else /* TESTING */
#define INIT_TIME(tok, current_time)                                           \
    do {                                                                       \
        assert(tok->time_fn != NULL);                                          \
        tok->time_fn(&current_time);                                           \
    } while (0)
#endif /* TESTING */

jaeger_token_bucket* jaeger_token_bucket_init(double credits_per_second,
                                              double max_balance)
{
    jaeger_token_bucket* tok =
        jaeger_global_alloc_malloc(sizeof(jaeger_token_bucket));
    if (tok == NULL) {
        fprintf(stderr, "ERROR: Cannot allocate jaeger_token_bucket\n");
        return NULL;
    }
#ifndef TESTING
    tok->time_fn = &jaeger_duration_now;
#endif /* TESTING */
    tok->credits_per_second = credits_per_second;
    tok->max_balance = max_balance;
    INIT_TIME(tok, tok->last_tick);
    return tok;
}

static void update_balance(jaeger_token_bucket* tok)
{
    assert(tok != NULL);
    jaeger_duration current_time;
    INIT_TIME(tok, current_time);
    jaeger_duration interval;
    const int result =
        jaeger_duration_subtract(&current_time, &tok->last_tick, &interval);
    (void)result;
    assert(result == 0);
    const double diff =
        (((double)interval.tv_nsec) / NS_PER_S + interval.tv_sec) *
        tok->credits_per_second;
    tok->balance = MIN(tok->max_balance, tok->balance + diff);
    tok->last_tick = current_time;
}

bool jaeger_token_bucket_check_credit(jaeger_token_bucket* tok, double cost)
{
    update_balance(tok);
    if (tok->balance < cost) {
        return 0;
    }
    tok->balance -= cost;
    return 1;
}
