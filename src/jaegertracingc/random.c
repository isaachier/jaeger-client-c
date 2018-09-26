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

#include "jaegertracingc/random.h"

jaeger_thread_local rng_storage = {.initialized = false};

static jaeger_once once = JAEGERTRACINGC_ONCE_INIT;

void cleanup_rng(void)
{
    jaeger_thread_local_destroy(&rng_storage);
}

void init_rng(void)
{
    jaeger_thread_local_init(&rng_storage);
    atexit(&cleanup_rng);
}

#ifdef HAVE_ARC4RANDOM

static inline uint64_t random64(const unsigned int* state)
{
    (void) state;
    uint64_t result;
    arc4random_buf(&result, sizeof(result));
    return result;
}

#else

static inline uint64_t random64(unsigned int* state)
{
    return (((uint64_t) rand_r(state)) << 32u) | rand_r(state);
}

#endif /* HAVE_ARC4RANDOM */

uint64_t jaeger_random64(void)
{
    jaeger_do_once(&once, init_rng);
    assert(rng_storage.initialized);
    jaeger_rng* rng = (jaeger_rng*) jaeger_thread_local_get_value(&rng_storage);
    if (rng == NULL) {
        rng = jaeger_rng_alloc();
        if (rng == NULL) {
            goto cleanup;
        }
        jaeger_rng_init(rng);
        if (!jaeger_thread_local_set_value(&rng_storage,
                                           (jaeger_destructible*) rng)) {
            goto cleanup;
        }
    }

    assert(rng != NULL);
    return random64(&rng->state);

cleanup:
    jaeger_free(rng);
    return 0;
}
