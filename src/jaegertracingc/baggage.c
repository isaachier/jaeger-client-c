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

#include "jaegertracingc/baggage.h"

#include "jaegertracingc/tracer.h"

#define LOG_FIELD(key_str, value_str)              \
    {                                              \
        .key = (key_str), .value = {               \
            .type = opentracing_value_string,      \
            .value = {.string_value = (value_str)} \
        }                                          \
    }

#define BOOL_STR(x) ((x) ? "true" : "false")

jaeger_baggage_restriction
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

void jaeger_default_baggage_restriction_manager_init(
    jaeger_default_baggage_restriction_manager* manager, size_t max_value_len)
{
    *manager = (jaeger_default_baggage_restriction_manager){
        .base =
            {.get_restriction =
                 jaeger_default_baggage_restriction_manager_get_restriction},
        .max_value_len = max_value_len};
}

void jaeger_baggage_setter_set_baggage(jaeger_baggage_setter* setter,
                                       jaeger_span* span,
                                       const char* key,
                                       const char* value)
{
    assert(setter != NULL);

    bool truncated = false;
    bool prev_item = false;

    const jaeger_baggage_restriction restriction =
        setter->manager->get_restriction(
            setter->manager, span->tracer->service_name, key);
    if (!restriction.key_allowed) {
        goto log;
    }

    const size_t value_len = strlen(value);
    truncated = (value_len > restriction.max_value_len);
    if (truncated) {
        if (setter->metrics != NULL) {
            setter->metrics->baggage_truncate->inc(
                setter->metrics->baggage_truncate, 1);
        }
    }

    const jaeger_key_value* kv =
        jaeger_hashtable_find(&span->context.baggage, key);
    prev_item = (kv != NULL);
    if (truncated) {
        char value_copy[restriction.max_value_len + 1];
        strncpy(value_copy, value, sizeof(value_copy) - 1);
        value_copy[restriction.max_value_len] = '\0';
        jaeger_hashtable_put(&span->context.baggage, key, value_copy);
    }
    else {
        jaeger_hashtable_put(&span->context.baggage, key, value);
    }

log:
    if (jaeger_span_is_sampled_no_locking(span)) {
        opentracing_log_field fields[] = {
            LOG_FIELD("event", "baggage"),
            LOG_FIELD("key", key),
            LOG_FIELD("value", value),
            LOG_FIELD("override", BOOL_STR(prev_item)),
            LOG_FIELD("truncated", BOOL_STR(truncated)),
            LOG_FIELD("invalid", BOOL_STR(!restriction.key_allowed))};
        jaeger_timestamp timestamp;
        jaeger_timestamp_now(&timestamp);
        const opentracing_log_record log_record = {
            .fields = fields,
            .num_fields = sizeof(fields) / sizeof(fields[0]),
            .timestamp = timestamp};
        jaeger_span_log_no_locking(span, &log_record);
    }
}
