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

#ifndef JAEGERTRACINGC_INTERNAL_TEST_HELPERS_H
#define JAEGERTRACINGC_INTERNAL_TEST_HELPERS_H

#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

static inline char random_char()
{
    switch (rand() % 3) {
    case 0:
        return 'a' + rand() % 26;
    case 1:
        return 'A' + rand() % 26;
    default:
        return '0' + rand() % 10;
    }
}

static inline void random_string(char* buffer, size_t len)
{
    for (int i = 0; i < ((int) len) - 1; i++) {
        buffer[i] = random_char();
    }
    buffer[len - 1] = '\0';
}

#ifdef __cplusplus
} /* extern C */
#endif /* __cplusplus */

#endif /* JAEGERTRACINGC_INTERNAL_TEST_HELPERS_H */
