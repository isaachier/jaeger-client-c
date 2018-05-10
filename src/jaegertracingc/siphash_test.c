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
    } test_cases[] = {{.data = "test", .hash_code = 0x3d5124c4cd58914e},
                      {.data = "12341234", .hash_code = 0x47a9ed4e01555590},
                      {.data = "1", .hash_code = 0x45712a3dbb8d63d2},
                      {.data = "2", .hash_code = 0x9ee4f4d2aca06b87},
                      {.data = "3", .hash_code = 0x32f8839d8f6766a5},
                      {.data = "4", .hash_code = 0x4b08d0a44f1bfb70},
                      {.data = "5", .hash_code = 0xc8f6efcbad548143},
                      {.data = "6", .hash_code = 0x1657adc431e01cbd},
                      {.data = "7", .hash_code = 0x7291fb2a1da09ec9},
                      {.data = "8", .hash_code = 0x75d748ca73747612},
                      {.data = "9", .hash_code = 0x88790356b7346f2},
                      {.data = "10", .hash_code = 0xb85bb114e4d30b40}};
    for (size_t i = 0, len = sizeof(test_cases) / sizeof(test_cases[0]);
         i < len;
         i++) {
        TEST_ASSERT_EQUAL_HEX64(test_cases[i].hash_code,
                                siphash((uint8_t*) test_cases[i].data,
                                        strlen(test_cases[i].data),
                                        seed));
    }
}
