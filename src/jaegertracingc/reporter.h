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

#ifndef JAEGERTRACINGC_REPORTER_H
#define JAEGERTRACINGC_REPORTER_H

#include "jaegertracingc/common.h"
#include "jaegertracingc/logging.h"
#include "jaegertracingc/metrics.h"
#include "jaegertracingc/net.h"
#include "jaegertracingc/span.h"
#include "jaegertracingc/threading.h"
#include "jaegertracingc/vector.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define JAEGERTRACINGC_DEFAULT_UDP_SPAN_SERVER_HOST_PORT "localhost:6831"

#define JAEGERTRACINGC_DEFAULT_UDP_BUFFER_SIZE USHRT_MAX

#define JAEGERTRACINGC_REPORTER_SUBCLASS              \
    JAEGERTRACINGC_DESTRUCTIBLE_SUBCLASS;             \
    void (*report)(struct jaeger_reporter * reporter, \
                   const jaeger_span* span,           \
                   jaeger_logger* logger)

typedef struct jaeger_reporter {
    JAEGERTRACINGC_REPORTER_SUBCLASS;
} jaeger_reporter;

/* Shared instance of null reporter. DO NOT MODIFY MEMBERS! */
jaeger_reporter* jaeger_null_reporter();

void jaeger_logging_reporter_init(jaeger_reporter* reporter);

typedef struct jaeger_in_memory_reporter {
    JAEGERTRACINGC_REPORTER_SUBCLASS;
    jaeger_vector spans;
    jaeger_mutex mutex;
} jaeger_in_memory_reporter;

bool jaeger_in_memory_reporter_init(jaeger_in_memory_reporter* reporter,
                                    jaeger_logger* logger);

typedef struct jaeger_composite_reporter {
    JAEGERTRACINGC_REPORTER_SUBCLASS;
    jaeger_vector reporters;
} jaeger_composite_reporter;

bool jaeger_composite_reporter_init(jaeger_composite_reporter* reporter,
                                    jaeger_logger* logger);

static inline bool
jaeger_composite_reporter_add(jaeger_composite_reporter* reporter,
                              jaeger_reporter* new_reporter,
                              jaeger_logger* logger)
{
    assert(reporter != NULL);
    assert(new_reporter != NULL);
    jaeger_reporter** reporter_ptr =
        jaeger_vector_append(&reporter->reporters, logger);
    if (reporter_ptr == NULL) {
        return false;
    }
    *reporter_ptr = new_reporter;
    return true;
}

typedef struct jaeger_remote_reporter {
    JAEGERTRACINGC_REPORTER_SUBCLASS;
    int max_packet_size;
    int fd;
    jaeger_metrics* metrics;
    Jaegertracing__Protobuf__Process process;
    jaeger_vector spans;
} jaeger_remote_reporter;

bool jaeger_remote_reporter_init(jaeger_remote_reporter* reporter,
                                 const char* host_port,
                                 int max_packet_size,
                                 jaeger_metrics* metrics,
                                 jaeger_logger* logger);

int jaeger_remote_reporter_flush(jaeger_remote_reporter* reporter,
                                 jaeger_logger* logger);

#ifdef __cplusplus
} /* extern C */
#endif /* __cplusplus */

#endif /* JAEGERTRACINGC_REPORTER_H */
