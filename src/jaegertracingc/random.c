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
#include <linux/random.h>
#include <sys/syscall.h>
#include <unistd.h>
#endif /* HAVE_GETRANDOM */

#include "pcg_variants.h"

#include "jaegertracingc/alloc.h"
#include "jaegertracingc/logging.h"
#include "jaegertracingc/threading.h"

#define NUM_UINT64S_IN_STATE 16

static jaeger_thread_local rng_storage = {.initialized = false};

typedef struct jaeger_rng {
    JAEGERTRACINGC_DESTRUCTIBLE_SUBCLASS;
    struct pcg_state_setseq_64 state;
} jaeger_rng;

static void rng_destroy(jaeger_destructible* d)
{
    if (d == NULL) {
        return;
    }
    jaeger_free(d);
}

static inline void jaeger_rng_init(jaeger_rng* rng, jaeger_logger* logger)
{
    assert(rng != NULL);
    rng->destroy = &rng_destroy;
    uint64_t seed[2] = {0};
    const int num_requested = sizeof(seed);
#ifdef HAVE_GETRANDOM
    const int num_read =
        syscall(SYS_getrandom, &seed, num_requested, GRND_NONBLOCK);
#elif defined(HAVE_ARC4RANDOM)
    const int num_read = arc4random(&seed, num_requested);
#else
    int num_read = 0;
    FILE* random_device = fopen("/dev/random", "rb");
    if (random_device == NULL) {
        if (logger != NULL) {
            logger->warn(logger,
                         "Cannot open /dev/random to initialize random seed, "
                         "errno = %d",
                         errno);
        }
        num_read = -1;
    }
    num_read = fread(&seed,
                     sizeof(uint64_t),
                     num_requested / sizeof(uint64_t),
                     random_device);
    fclose(random_device);
#endif /* HAVE_GETRANDOM */
       /* Warn if we could not read entire seed from random source, but not an
        * error regardless. */
    if (num_read >= 0 && num_read != num_requested && logger != NULL) {
        logger->warn(logger,
                     "Could not read entire random block, "
                     "bytes requested = %d, bytes read = %d, errno = %d",
                     num_requested,
                     num_read,
                     errno);
    }
    else if (num_read == -1) {
        for (int i = 0; i < num_requested; i++) {
            for (int j = 0; j < sizeof(seed[0]); j++) {
                uint8_t byte = rand() & 0xff;
                seed[i] <<= CHAR_BIT;
                seed[i] |= byte;
            }
        }
    }

    pcg32_srandom_r(&rng->state, seed[0], seed[1]);
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
        jaeger_rng_init(rng, logger);
        if (!jaeger_thread_local_set_value(
                &rng_storage, (jaeger_destructible*) rng, logger)) {
            jaeger_free(rng);
            return 0;
        }
    }

    assert(rng != NULL);
    int64_t result = pcg32_random_r(&rng->state);
    result <<= 32;
    result |= pcg32_random_r(&rng->state);
    return result;
}
