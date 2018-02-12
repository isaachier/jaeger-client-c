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

static void null_report(jaeger_reporter* reporter, const jaeger_span* span)
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
