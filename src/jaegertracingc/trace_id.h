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

#ifndef JAEGERTRACINGC_TRACE_ID_H
#define JAEGERTRACINGC_TRACE_ID_H

#include "jaegertracingc/common.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct jaeger_trace_id {
    uint64_t high;
    uint64_t low;
} jaeger_trace_id;

#define JAEGERTRACINGC_TRACE_ID_INIT \
    {                                \
        .high = 0, .low = 0          \
    }

#define JAEGERTRACINGC_TRACE_ID_MAX_STR_LEN 33

static inline int jaeger_trace_id_format(const jaeger_trace_id* trace_id,
                                         char* buffer,
                                         int buffer_len)
{
    assert(trace_id != NULL);
    assert(buffer != NULL);
    assert(buffer_len >= 0);
    if (trace_id->high == 0) {
        return snprintf(buffer, buffer_len, "%lx", trace_id->low);
    }
    return snprintf(
        buffer, buffer_len, "%lx%016lx", trace_id->high, trace_id->low);
}

static inline const char* jaeger_trace_id_scan(jaeger_trace_id* trace_id,
                                               const char* first,
                                               const char* last)
{
#define JAEGERTRACINGC_UINT64_MAX_STR_LEN 16
#define JAEGERTRACINGC_HEX_BASE 16
    assert(trace_id != NULL);
    assert(first != NULL);
    assert(last != NULL);
    uint64_t high = 0;
    uint64_t low = 0;
    const int len = last - first;
    char* iter = NULL;
    if (len <= JAEGERTRACINGC_UINT64_MAX_STR_LEN) {
        iter = (char*) last;
        low = strtoull(first, &iter, JAEGERTRACINGC_HEX_BASE);
    }
    else {
        const int high_len = len - JAEGERTRACINGC_UINT64_MAX_STR_LEN;
        char high_str[JAEGERTRACINGC_UINT64_MAX_STR_LEN];
        memcpy(high_str, first, high_len);
        high_str[high_len] = '\0';
        iter = &high_str[high_len];
        high = strtoull(high_str, &iter, JAEGERTRACINGC_HEX_BASE);
        if (iter != &high_str[high_len]) {
            const int offset = iter - high_str;
            return first + offset;
        }
        iter = (char*) last;
        low = strtoull(last - JAEGERTRACINGC_UINT64_MAX_STR_LEN,
                       &iter,
                       JAEGERTRACINGC_HEX_BASE);
    }
    if (iter != last) {
        return iter;
    }
    *trace_id = (jaeger_trace_id){.high = high, .low = low};
    return last;
}

#ifdef __cplusplus
} /* extern C */
#endif /* __cplusplus */

#endif /* JAEGERTRACINGC_TRACE_ID_H */
