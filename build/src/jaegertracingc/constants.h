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

#ifndef JAEGERTRACINGC_CONSTANTS_H
#define JAEGERTRACINGC_CONSTANTS_H

#include "jaegertracingc/common.h"

#define JAEGERTRACINGC_CLIENT_VERSION "C-0.0.1"
#define JAEGERTRACINGC_CLIENT_VERSION_TAG_KEY "jaeger.version"
#define JAEGERTRACINGC_DEBUG_HEADER "jaeger-debug-id"
#define JAEGERTRACINGC_BAGGAGE_HEADER "jaeger-baggage"
#define JAEGERTRACINGC_TRACER_HOSTNAME_TAG_KEY "hostname"
#define JAEGERTRACINGC_TRACER_IP_TAG_KEY "ip"
#define JAEGERTRACINGC_SAMPLER_TYPE_TAG_KEY "sampler.type"
#define JAEGERTRACINGC_SAMPLER_PARAM_TAG_KEY "sampler.param"
#define JAEGERTRACINGC_TRACE_CONTEXT_HEADER_NAME "uber-trace-id"
#define JAEGERTRACINGC_TRACER_STATE_HEADER_NAME \
    JAEGERTRACINGC_TRACE_CONTEXT_HEADER_NAME
#define JAEGERTRACINGC_TRACE_BAGGAGE_HEADER_PREFIX "uberctx-"
#define JAEGERTRACINGC_SAMPLER_TYPE_CONST "const"
#define JAEGERTRACINGC_SAMPLER_TYPE_REMOTE "remote"
#define JAEGERTRACINGC_SAMPLER_TYPE_PROBABILISTIC "probabilistic"
#define JAEGERTRACINGC_SAMPLER_TYPE_RATE_LIMITING "ratelimiting"
#define JAEGERTRACINGC_SAMPLER_TYPE_LOWER_BOUND "lowerbound"

#endif  // JAEGERTRACINGC_CONSTANTS_H
