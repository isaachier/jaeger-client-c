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

#ifndef JAEGERTRACINGC_PROPAGATION_H
#define JAEGERTRACINGC_PROPAGATION_H

#include "jaegertracingc/common.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define JAEGERTRACINGC_TEXT_MAP_READER_SUBCLASS                          \
    JAEGERTRACINGC_DESTRUCTIBLE_SUBCLASS;                                \
    void (*for_each_key)(                                                \
        struct jaeger_text_map_reader * reader,                          \
        void (*callback)(void* arg, const char* key, const char* value), \
        void* arg);                                                      \
    const char* (*lookup_key)(struct jaeger_text_map_reader * reader,    \
                              const char* key)

typedef struct jaeger_text_map_reader {
    JAEGERTRACINGC_TEXT_MAP_READER_SUBCLASS;
} jaeger_text_map_reader;

#define JAEGERTRACINGC_TEXT_MAP_WRITER_SUBCLASS         \
    JAEGERTRACINGC_DESTRUCTIBLE_SUBCLASS;               \
    void (*set)(struct jaeger_text_map_writer * writer, \
                const char* key,                        \
                const char* value)

typedef struct jaeger_text_map_writer {
    JAEGERTRACINGC_TEXT_MAP_WRITER_SUBCLASS;
} jaeger_text_map_writer;

#ifdef __cplusplus
} /* extern C */
#endif /* __cplusplus */

#endif /* JAEGERTRACINGC_PROPAGATION_H */
