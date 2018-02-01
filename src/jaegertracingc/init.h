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

#ifndef JAEGERTRACINGC_INIT_H
#define JAEGERTRACINGC_INIT_H

#include "jaegertracingc/alloc.h"
#include "jaegertracingc/common.h"

/* Use this function to initialize the library before calling any functions.
 * Either pass in a custom allocator or NULL to use the built-in allocator. */
void jaeger_init_lib(jaeger_allocator* alloc);

#endif /* JAEGERTRACINGC_INIT_H */
