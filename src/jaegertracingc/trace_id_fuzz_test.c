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

#include "jaegertracingc/trace_id.h"

int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    uint8_t buffer[size + 1];
    memcpy(buffer, data, size);
    buffer[size] = '\0';
    jaeger_trace_id trace_id;
    jaeger_trace_id_scan(&trace_id, (const char*) buffer);
    return 0;
}
