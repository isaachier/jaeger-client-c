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

#ifndef JAEGERTRACINGC_INTERNAL_RANDOM_H
#define JAEGERTRACINGC_INTERNAL_RANDOM_H

#include <errno.h>

#include "pcg_variants.h"

#include "jaegertracingc/common.h"
#include "jaegertracingc/random.h"
#include "jaegertracingc/threading.h"

#define NUM_UINT64_IN_SEED 2

#if defined(HAVE_GETRANDOM)

#include <linux/random.h>
#include <sys/syscall.h>
#include <unistd.h>

#define RANDOM_SEED(seed) \
    syscall(SYS_getrandom, seed, sizeof(seed), GRND_NONBLOCK)

#elif defined(HAVE_ARC4RANDOM_BUF)

#define RANDOM_SEED(seed) arc4random_buf(seed, sizeof(seed))

#else

#define RANDOM_SEED(seed) read_random_seed(seed, "/dev/random")

#endif /* HAVE_GETRANDOM */

extern jaeger_thread_local rng_storage;

typedef struct jaeger_rng {
    JAEGERTRACINGC_DESTRUCTIBLE_SUBCLASS;
    struct pcg_state_setseq_64 state;
} jaeger_rng;

static inline void read_random_seed(uint64_t* seed,
                                    const char* random_source_path)
{
    FILE* random_source_file = fopen(random_source_path, "r");
    if (random_source_file == NULL) {
        jaeger_log_warn("Cannot open %s to initialize random seed, "
                        "errno = %d",
                        random_source_path,
                        errno);
        return;
    }

    const int num_read =
        fread(seed, sizeof(seed[0]), NUM_UINT64_IN_SEED, random_source_file);
    fclose(random_source_file);

    /* Warn if we could not read entire seed from random source, but not an
     * error regardless. */
    if (num_read != NUM_UINT64_IN_SEED) {
        jaeger_log_warn("Could not read entire random block, "
                        "bytes requested = %lu, bytes read = %lu, errno = %d",
                        NUM_UINT64_IN_SEED * sizeof(uint64_t),
                        num_read * sizeof(uint64_t),
                        errno);
    }
}

static void rng_destroy(jaeger_destructible* d)
{
    if (d == NULL) {
        return;
    }
    jaeger_rng* rng = (jaeger_rng*) d;
    jaeger_free(rng);
}

static inline void jaeger_rng_init(jaeger_rng* rng)
{
    assert(rng != NULL);
    rng->destroy = &rng_destroy;
    uint64_t seed[NUM_UINT64_IN_SEED];
    memset(seed, 0, sizeof(seed));
    RANDOM_SEED(seed);
    pcg32_srandom_r(&rng->state, seed[0], seed[1]);
}

static inline jaeger_rng* jaeger_rng_alloc(void)
{
    jaeger_rng* rng = (jaeger_rng*) jaeger_malloc(sizeof(jaeger_rng));
    if (rng == NULL) {
        jaeger_log_error("Cannot allocate random number generator");
    }
    return rng;
}

#endif /* JAEGERTRACINGC_INTERNAL_RANDOM_H */
