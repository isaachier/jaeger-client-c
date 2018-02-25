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

#include "init.h"
#include "alloc.h"
#include "logging.h"
#include "unit_test_driver.h"

int main()
{
    jaeger_logger* logger = jaeger_null_logger();
    jaeger_allocator* alloc = jaeger_null_allocator();
    TEST_ASSERT_NOT_NULL(logger);
    TEST_ASSERT_NOT_NULL(alloc);
    jaeger_init_lib(logger, alloc);
    TEST_ASSERT_EQUAL(alloc, jaeger_default_allocator());
    TEST_ASSERT_EQUAL(logger, jaeger_default_logger());
    return 0;
}
