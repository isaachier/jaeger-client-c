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

#ifndef JAEGERTRACINGC_KEY_VALUE_H
#define JAEGERTRACINGC_KEY_VALUE_H

#include "jaegertracingc/common.h"

typedef struct jaeger_key_value {
    sds key;
    sds value;
} jaeger_key_value;

typedef struct jaeger_key_value_list {
    jaeger_key_value* kv;
    int size;
    int capacity;
} jaeger_key_value_list;

bool jaeger_key_value_list_init(jaeger_key_value_list* list);

bool jaeger_key_value_list_append(
    jaeger_key_value_list* list, const char* key, const char* value);

void jaeger_key_value_list_free(jaeger_key_value_list* list);

#endif  // JAEGERTRACINGC_KEY_VALUE_H
