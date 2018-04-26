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

/**
 * @file
 * @internal
 * String utilities.
 */

#ifndef JAEGERTRACINGC_INTERNAL_STRINGS_H
#define JAEGERTRACINGC_INTERNAL_STRINGS_H

#include <assert.h>
#include <ctype.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline int decode_hex(char ch)
{
    if (islower(ch)) {
        if (ch > 'f') {
            return -1;
        }
        return (ch - 'a') + 10;
    }
    if (isupper(ch)) {
        if (ch > 'F') {
            return -1;
        }
        return (ch - 'A') + 10;
    }
    if (isdigit(ch)) {
        return ch - '0';
    }
    return -1;
}

static inline void decode_uri_value(char* restrict dst,
                                    const char* restrict src)
{
    enum state { default_state, percent_state, first_hex_state };

    enum state state = default_state;
    const int src_len = strlen(src);
    int dst_len = 0;
    int first_nibble = 0;
    int second_nibble = 0;

#define APPEND_CHAR(x)                      \
    do {                                    \
        assert(dst_len + 1 <= src_len + 1); \
        assert((x) < 0x7f);                 \
        dst[dst_len] = (x);                 \
        dst_len++;                          \
    } while (0)

    for (int i = 0; i < src_len; i++) {
        char ch = src[i];
        switch (state) {
        case default_state:
            if (ch == '%') {
                state = percent_state;
            }
            else {
                APPEND_CHAR(ch);
            }
            break;
        case percent_state:
            first_nibble = decode_hex(ch);
            if (first_nibble == -1) {
                APPEND_CHAR('%');
                APPEND_CHAR(ch);
                state = default_state;
            }
            else {
                state = first_hex_state;
            }
            break;
        default:
            assert(state == first_hex_state);
            second_nibble = decode_hex(ch);
            if (second_nibble == -1) {
                APPEND_CHAR('%');
                APPEND_CHAR((char) first_nibble);
                APPEND_CHAR(ch);
            }
            else {
                APPEND_CHAR((char) (((first_nibble & 0xf) << 4) |
                                    (second_nibble & 0xf)));
            }
            state = default_state;
            break;
        }
    }
    switch (state) {
    case percent_state:
        APPEND_CHAR('%');
        break;
    case first_hex_state:
        APPEND_CHAR('%');
        APPEND_CHAR((char) first_nibble);
        break;
    default:
        break;
    }
    APPEND_CHAR('\0');

#undef APPEND_CHAR
}

static inline void copy_str(char* restrict dst, const char* restrict src)
{
    strncpy(dst, src, strlen(src) + 1);
}

static inline void to_lowercase(char* restrict dst, const char* restrict src)
{
    const int len = strlen(src);
    for (int i = 0; i < len; i++) {
        dst[i] = tolower(src[i]);
    }
    dst[len] = '\0';
}

#ifdef __cplusplus
}
#endif

#endif /* JAEGERTRACINGC_INTERNAL_STRINGS_H */
