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

/**
 * @file
 * Baggage restriction utilities.
 */

#ifndef JAEGERTRACINGC_BAGGAGE_H
#define JAEGERTRACINGC_BAGGAGE_H

#include "jaegertracingc/common.h"
#include "jaegertracingc/metrics.h"
#include "jaegertracingc/span.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * Representation of baggage restriction for a given key and a given service.
 */
typedef struct jaeger_baggage_restriction {
    /** Whether or not the key is allowed at all. */
    bool key_allowed;
    /** Maximum value length for the given key. */
    size_t max_value_len;
} jaeger_baggage_restriction;

/**
 * Interface for object that manages baggage restrictions.
 */
typedef struct jaeger_baggage_restriction_manager {
    /**
     * Get restriction regarding a given key for a given service.
     * @param manager Manager instance.
     * @param service Service name.
     * @param key Baggage key.
     * @return Baggage restriction value.
     */
    jaeger_baggage_restriction (*get_restriction)(
        struct jaeger_baggage_restriction_manager* manager,
        const char* service,
        const char* key);
} jaeger_baggage_restriction_manager;

/**
 * Default implementation of jaeger_baggage_restriction_manager interface.
 * @extends jaeger_baggage_restriction_manager
 */
typedef struct jaeger_default_baggage_restriction_manager {
    /** Base class instance. */
    jaeger_baggage_restriction_manager base;

    /** Maximum value length for every key. */
    size_t max_value_len;
} jaeger_default_baggage_restriction_manager;

/**
 * Implementation of get_restriction for
 * jaeger_default_baggage_restriction_manager.
 */
static inline jaeger_baggage_restriction
jaeger_default_baggage_restriction_manager_get_restriction(
    jaeger_baggage_restriction_manager* manager,
    const char* service,
    const char* key)
{
    (void) service;
    (void) key;
    jaeger_default_baggage_restriction_manager* default_manager =
        (jaeger_default_baggage_restriction_manager*) manager;
    return (jaeger_baggage_restriction){
        .key_allowed = true, .max_value_len = default_manager->max_value_len};
}

/**
 * Initialize an instance of jaeger_default_baggage_restriction_manager.
 * @param manager Manager instance.
 * @param max_value_len Maximum value length for all keys.
 */
static inline void jaeger_default_baggage_restriction_manager_init(
    jaeger_default_baggage_restriction_manager* manager, size_t max_value_len)
{
    *manager = (jaeger_default_baggage_restriction_manager){
        .base =
            {.get_restriction =
                 jaeger_default_baggage_restriction_manager_get_restriction},
        .max_value_len = max_value_len};
}

/** Facade to enforce remote baggage restrictions in tracer. */
typedef struct jaeger_baggage_setter {
    /** Remote baggage restriction manager instance. */
    jaeger_baggage_restriction_manager* manager;
    /** Metrics used for maintaining success count, error count, etc. */
    jaeger_metrics* metrics;
} jaeger_baggage_setter;

void jaeger_baggage_setter_set_baggage(jaeger_baggage_setter* setter,
                                       jaeger_span* span,
                                       const char* key,
                                       const char* value);

#ifdef __cplusplus
} /* extern C */
#endif /* __cplusplus */

#endif /* JAEGERTRACINGC_BAGGAGE_H */
