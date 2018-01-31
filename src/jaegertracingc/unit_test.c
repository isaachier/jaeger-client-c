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

#include "alloc_test.h"
#include "duration_test.h"
#include "init.h"
#include "key_value_test.h"
#include "metrics_test.h"
#include "token_bucket_test.h"
#include "unity.h"

int main()
{
    jaeger_init_lib(NULL);
    RUN_TEST(test_alloc);
    RUN_TEST(test_duration);
    RUN_TEST(test_key_value);
    RUN_TEST(test_metrics);
    RUN_TEST(test_token_bucket);
    return 0;
}
