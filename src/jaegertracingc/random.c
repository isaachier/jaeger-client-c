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

#include "jaegertracingc/random.h"

#include <errno.h>
#ifdef HAVE_GETRANDOM
#include <sys/random.h>
#endif /* HAVE_GETRANDOM */

#include "jaegertracingc/random.h"

#include "jaegertracingc/alloc.h"
#include "jaegertracingc/logging.h"
#include "jaegertracingc/threading.h"

#define NUM_UINT64S_IN_STATE 16

static jaeger_thread_local rng_storage = {.initialized = false};

typedef struct jaeger_rng {
    JAEGERTRACINGC_DESTRUCTIBLE_SUBCLASS;
    uint64_t state[16];
    int index;
} jaeger_rng;

static void rng_destroy(jaeger_destructible* d)
{
    if (d == NULL) {
        return;
    }
    jaeger_free(d);
}

static inline bool jaeger_rng_init(jaeger_rng* rng, jaeger_logger* logger)
{
    assert(rng != NULL);
    rng->destroy = &rng_destroy;
    rng->index = 0;
    const int num_requested = sizeof(rng->state);
#ifdef HAVE_GETRANDOM
    const int num_read = getrandom(rng->state, num_requested, GRND_NONBLOCK);
#else
    FILE* random_device = fopen("/dev/random", "rb");
    if (random_device == NULL) {
        if (logger != NULL) {
            logger->error(logger,
                          "Cannot open /dev/random to initialize random state, "
                          "errno = %d",
                          errno);
        }
        return false;
    }
    const int num_read = fread(rng->state,
                               sizeof(uint64_t),
                               num_requested / sizeof(uint64_t),
                               random_device);
    fclose(random_device);
#endif /* HAVE_GETRANDOM */
    /* Warn if we could not read entire state from random source, but not an
     * error regardless. */
    if (num_read != num_requested && logger != NULL) {
        logger->warn(logger,
                     "Could not read entire random block, "
                     "bytes requested = %d, bytes read = %d, errno = %d",
                     num_requested,
                     num_read,
                     errno);
    }
    return true;
}

static inline jaeger_rng* jaeger_rng_alloc(jaeger_logger* logger)
{
    jaeger_rng* rng = jaeger_malloc(sizeof(jaeger_rng));
    if (rng == NULL && logger != NULL) {
        logger->error(logger, "Cannot allocate random number generator");
    }
    return rng;
}

int64_t random64(jaeger_logger* logger)
{
    if (!rng_storage.initialized &&
        !jaeger_thread_local_init(&rng_storage, logger)) {
        return 0;
    }
    assert(rng_storage.initialized);
    jaeger_rng* rng =
        (jaeger_rng*) jaeger_thread_local_get_value(&rng_storage, logger);
    if (rng == NULL) {
        rng = jaeger_rng_alloc(logger);
        if (rng == NULL) {
            return 0;
        }
        if (!jaeger_rng_init(rng, logger) ||
            !jaeger_thread_local_set_value(
                &rng_storage, (jaeger_destructible*) rng, logger)) {
            jaeger_free(rng);
            return 0;
        }
    }

    assert(rng != NULL);
    /* Based on xorshift1024star from https://en.wikipedia.org/wiki/Xorshift. */
    const uint64_t s0 = rng->state[rng->index];
    rng->index = (rng->index + 1) & 15;
    uint64_t s1 = rng->state[rng->index];
    s1 ^= s1 << 31;
    s1 ^= s1 >> 11;
    s1 ^= s0 ^ (s0 >> 30);
    rng->state[rng->index] = s1;
    return s1 * ((uint64_t) 1181783497276652981);
}
