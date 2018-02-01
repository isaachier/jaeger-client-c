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

#include "jaegertracingc/key_value.h"
#include "unity.h"
#include <stdio.h>
#include <string.h>

void test_key_value()
{
    jaeger_key_value_list kv = {0};
    bool result = jaeger_key_value_list_init(&kv);
    TEST_ASSERT_TRUE(result);
    for (int i = 0; i < JAEGERTRACINGC_KV_INIT_SIZE + 1; i++) {
        char buffer[16];
        memset(&buffer, 0, sizeof(buffer));
        snprintf(&buffer[0], sizeof(buffer), "%d", i);
        result = jaeger_key_value_list_append(&kv, &buffer[0], &buffer[0]);
        TEST_ASSERT_TRUE(result);
    }
    jaeger_key_value_list_free(&kv);
}
