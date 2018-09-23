/*
 * Copyright (c) 2018 The Jaeger Authors.
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
                      {.data = "10", .hash_code = 0xb85bb114e4d30b40},
                      {.data = "100", .hash_code = 0x51b74da82050ee7a},
                      {.data = "1000", .hash_code = 0xc2dcf31e3cea700d},
                      {.data = "10000", .hash_code = 0x1a858cc904f6b81c},
                      {.data = "100000", .hash_code = 0x7826d9e168cffab9},
                      {.data = "1000000", .hash_code = 0x1166ea84c410bf93},
                      {.data = "10000000", .hash_code = 0xb6a7d994d1d5da57},
                      {.data = "100000000", .hash_code = 0x161e2914c8c6ec7f}};
    for (size_t i = 0, len = sizeof(test_cases) / sizeof(test_cases[0]);
         i < len;
         i++) {
        TEST_ASSERT_EQUAL_HEX64(test_cases[i].hash_code,
                                jaeger_siphash((uint8_t*) test_cases[i].data,
                                               strlen(test_cases[i].data),
                                               seed));
    }
}
