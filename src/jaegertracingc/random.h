/*
 * Copyright (c) 2018 The Jaeger Authors.
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
 * Random number generation.
 */

#ifndef JAEGERTRACINGC_RANDOM_H
#define JAEGERTRACINGC_RANDOM_H

#include <errno.h>
#include <stdint.h>

#include "jaegertracingc/common.h"
#include "jaegertracingc/threading.h"

#if defined(HAVE_GETRANDOM)
#include <linux/random.h>
#include <sys/syscall.h>
#include <unistd.h>
#endif /* HAVE_GETRANDOM */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

extern jaeger_thread_local rng_storage;

typedef struct jaeger_rng {
    jaeger_destructible base;
    unsigned int state;
} jaeger_rng;

static inline void
read_random_seed(void* seed, size_t len, const char* random_source_path)
{
    FILE* random_source_file = fopen(random_source_path, "re");
    if (random_source_file == NULL) {
        jaeger_log_warn("Cannot open %s to initialize random seed, "
                        "errno = %d",
                        random_source_path,
                        errno);
        return;
    }

    const int num_read = fread((char*) seed, 1, len, random_source_file);
    fclose(random_source_file);

    /* Warn if we could not read entire seed from random source, but not an
     * error regardless. */
    if (num_read != (int) len) {
        jaeger_log_warn("Could not read entire random block, "
                        "bytes requested = %lu, bytes read = %lu, errno = %d",
                        len,
                        num_read,
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

#define NUM_UINT64_IN_SEED 2

static inline jaeger_rng* jaeger_rng_alloc(void)
{
    jaeger_rng* rng = (jaeger_rng*) jaeger_malloc(sizeof(jaeger_rng));
    if (rng == NULL) {
        jaeger_log_error("Cannot allocate random number generator");
    }
    return rng;
}

#if defined(HAVE_GETRANDOM)

static inline void random_seed(void* seed, size_t size)
{
    syscall(SYS_getrandom, (seed), (size), GRND_NONBLOCK);
}

#elif defined(HAVE_ARC4RANDOM_BUF)

static inline void random_seed(void* seed, size_t size)
{
    arc4random_buf((seed), (size));
}

#else

static inline void random_seed(void* seed, size_t size)
{
    read_random_seed((seed), (size), "/dev/random");
}

#endif /* HAVE_GETRANDOM */

static inline void jaeger_rng_init(jaeger_rng* rng)
{
    assert(rng != NULL);
    ((jaeger_destructible*) rng)->destroy = &rng_destroy;
    random_seed(&rng->state, sizeof(rng->state));
}

int64_t jaeger_random64(void);

#ifdef __cplusplus
} /* extern C */
#endif /* __cplusplus */

#endif /* JAEGERTRACINGC_RANDOM_H */
