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

#define JAEGERTRACINGC_TRACE_ID_MAX_STR_LEN 32
#define JAEGERTRACINGC_UINT64_MAX_STR_LEN 16
#define JAEGERTRACINGC_HEX_BASE 16

static inline int jaeger_trace_id_format(const jaeger_trace_id* trace_id,
                                         char* buffer,
                                         int buffer_len)
{
    assert(trace_id != NULL);
    assert(buffer != NULL);
    assert(buffer_len >= 0);
    if (trace_id->high == 0) {
        return snprintf(buffer, buffer_len, "%" PRIx64, trace_id->low);
    }
    return snprintf(buffer,
                    buffer_len,
                    "%" PRIx64 "%016" PRIx64,
                    trace_id->high,
                    trace_id->low);
}

static inline bool jaeger_trace_id_scan(jaeger_trace_id* trace_id,
                                        const char* str)
{
    assert(trace_id != NULL);
    assert(str != NULL);
    const int len = strlen(str);
    if (len > JAEGERTRACINGC_UINT64_MAX_STR_LEN * 2) {
        return false;
    }
    uint64_t high = 0;
    uint64_t low = 0;
    char* iter = NULL;
    if (len > JAEGERTRACINGC_UINT64_MAX_STR_LEN) {
        const char* low_start = str + len - JAEGERTRACINGC_UINT64_MAX_STR_LEN;
        const int high_len = low_start - str;
        char buffer[high_len + 1];
        memcpy(buffer, str, high_len);
        buffer[high_len] = '\0';
        high = strtoull(buffer, &iter, JAEGERTRACINGC_HEX_BASE);
        assert(iter != NULL);
        if (*iter != '\0') {
            return false;
        }
        iter = NULL;
        str = low_start;
    }
    low = strtoull(str, &iter, JAEGERTRACINGC_HEX_BASE);
    assert(iter != NULL);
    if (*iter != '\0') {
        return false;
    }
    *trace_id = (jaeger_trace_id){.high = high, .low = low};
    return true;
}

#ifdef __cplusplus
} /* extern C */
#endif /* __cplusplus */

#endif /* JAEGERTRACINGC_TRACE_ID_H */
