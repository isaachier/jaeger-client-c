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

#include "jaegertracingc/reporter.h"
#include "jaegertracingc/threading.h"

static jaeger_reporter null_reporter;

static void null_destroy(jaeger_destructible* destructible)
{
    (void) destructible;
}

static void null_report(jaeger_reporter* reporter,
                        const jaeger_span* span,
                        jaeger_logger* logger)
{
    (void) reporter;
    (void) span;
}

static void init_null_reporter()
{
    null_reporter =
        (jaeger_reporter){.destroy = &null_destroy, .report = &null_report};
}

jaeger_reporter* jaeger_null_reporter()
{
    static jaeger_once once = JAEGERTRACINGC_ONCE_INIT;
    jaeger_do_once(&once, &init_null_reporter);
    return &null_reporter;
}

static void logging_reporter_report(jaeger_reporter* reporter,
                                    const jaeger_span* span,
                                    jaeger_logger* logger)
{
    assert(reporter != NULL);
    if (logger != NULL && span != NULL) {
        char buffer[JAEGERTRACINGC_SPAN_CONTEXT_MAX_STR_LEN + 1];
        buffer[JAEGERTRACINGC_SPAN_CONTEXT_MAX_STR_LEN] = '\0';
        logger->info(logger, "%s", buffer);
    }
}

void jaeger_logging_reporter_init(jaeger_reporter* reporter)
{
    assert(reporter != NULL);
    reporter->destroy = &null_destroy;
    reporter->report = &logging_reporter_report;
}

static void in_memory_reporter_destroy(jaeger_destructible* destructible)
{
    if (destructible != NULL) {
        jaeger_in_memory_reporter* r =
            (jaeger_in_memory_reporter*) destructible;
        jaeger_vector_destroy(&r->spans);
    }
}

static void in_memory_reporter_report(jaeger_reporter* reporter,
                                      const jaeger_span* span,
                                      jaeger_logger* logger)
{
    assert(reporter != NULL);
    jaeger_in_memory_reporter* r = (jaeger_in_memory_reporter*) reporter;
    if (span != NULL) {
        jaeger_mutex_lock(&r->mutex);
        jaeger_span* span_copy = jaeger_vector_append(&r->spans, NULL);
        if (span_copy != NULL) {
            jaeger_span_copy(span_copy, span, logger);
        }
        jaeger_mutex_unlock(&r->mutex);
    }
}

bool jaeger_in_memory_reporter_init(jaeger_in_memory_reporter* reporter,
                                    jaeger_logger* logger)
{
    assert(reporter != NULL);
    if (!jaeger_vector_init(
            &reporter->spans, sizeof(jaeger_span), NULL, logger)) {
        return false;
    }
    reporter->destroy = &in_memory_reporter_destroy;
    reporter->report = &in_memory_reporter_report;
    reporter->mutex = (jaeger_mutex) JAEGERTRACINGC_MUTEX_INIT;
    return true;
}
