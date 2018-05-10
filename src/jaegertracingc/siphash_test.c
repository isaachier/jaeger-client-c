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

#include "jaegertracingc/siphash.h"
#include "unity.h"

void test_siphash()
{
    const uint8_t seed[16] = {0};
    const struct {
        const char* data;
        uint64_t hash_code;
    } test_cases[] = {{.data = "test", .hash_code = 0x3d5124c4cd58914e}};
    for (size_t i = 0, len = sizeof(test_cases) / sizeof(test_cases[0]);
         i < len;
         i++) {
        TEST_ASSERT_EQUAL_HEX64(
            test_cases[i].hash_code,
            siphash(test_cases[i].data, strlen(test_cases[i].data), seed));
    }
}
