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
#include "jaegertracingc/internal/random.h"

jaeger_thread_local rng_storage = {.initialized = false};

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
