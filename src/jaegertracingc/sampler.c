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

#include "jaegertracingc/sampler.h"

extern inline void jaeger_sampler_noop_close(jaeger_sampler *sampler);

extern inline bool jaeger_const_sampler_is_sampled(jaeger_sampler *sampler,
                                    const jaeger_trace_id *trace_id,
                                    const char *operation_name,
                                    jaeger_key_value_list *tags);

extern inline bool jaeger_probabilistic_sampler_is_sampled(jaeger_sampler *sampler,
                                            const jaeger_trace_id *trace_id,
                                            const char *operation_name,
                                            jaeger_key_value_list *tags);

extern inline bool jaeger_rate_limiting_sampler_is_sampled(jaeger_sampler *sampler,
                                            const jaeger_trace_id *trace_id,
                                            const char *operation_name,
                                            jaeger_key_value_list *tags);

extern inline bool
jaeger_guaranteed_throughput_probabilistic_sampler_is_sampled(
    jaeger_sampler *sampler,
    const jaeger_trace_id *trace_id,
    const char *operation_name,
    jaeger_key_value_list *tags);

extern inline void
jaeger_guaranteed_throughput_probabilistic_sampler_close(
    jaeger_sampler *sampler);
