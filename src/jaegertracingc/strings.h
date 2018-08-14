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

#ifndef JAEGERTRACINGC_STRINGS_H
#define JAEGERTRACINGC_STRINGS_H

#include <assert.h>
#include <ctype.h>
#include <string.h>

#include "jaegertracingc/config.h"
#include "jaegertracingc/hashtable.h"

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

static inline char encode_hex(int num)
{
    assert(num >= 0 && num < 16);
    if (num < 10) {
        return '0' + num;
    }
    return 'a' + (num - 10);
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
                APPEND_CHAR(encode_hex(first_nibble));
                APPEND_CHAR(ch);
            }
            else {
                APPEND_CHAR((char) (((((uint8_t) first_nibble) & 0xfu) << 4u) |
                                    (((uint8_t) second_nibble) & 0xfu)));
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
        APPEND_CHAR(encode_hex(first_nibble));
        break;
    default:
        break;
    }
    APPEND_CHAR('\0');

#undef APPEND_CHAR
}

static inline void encode_uri_value(char* restrict dst,
                                    const char* restrict src)
{
    int pos = 0;
    for (int i = 0; i < (int) strlen(src); i++) {
        const char ch = src[i];
        if (isalnum(ch)) {
            dst[pos++] = ch;
        }
        else {
            switch (ch) {
            case ';':
            case '/':
            case '?':
            case ':':
            case '@':
            case '&':
            case '=':
            case '+':
            case '$':
            case ',':
            case '-':
            case '_':
            case '.':
            case '!':
            case '~':
            case '*':
            case '\'':
            case '(':
            case ')':
                dst[pos++] = ch;
                break;
            default: {
                dst[pos++] = '%';
                const uint8_t first_nibble = (((uint32_t) ch) >> 4u) & 0x0fu;
                const uint8_t second_nibble = ((uint8_t) ch) & 0x0fu;
                dst[pos++] = encode_hex(first_nibble);
                dst[pos++] = encode_hex(second_nibble);
            } break;
            }
        }
    }
    dst[pos] = '\0';
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

static inline opentracing_propagation_error_code
parse_key_value(jaeger_hashtable* baggage, char* str)
{
    assert(baggage != NULL);
    assert(str != NULL);
    char* eq_context;
    const char* key = strtok_r(str, "=", &eq_context);
    const char* value = strtok_r(NULL, "=", &eq_context);
    if (key == NULL || value == NULL) {
        return opentracing_propagation_error_code_span_context_corrupted;
    }

    if (!jaeger_hashtable_put(baggage, key, value)) {
        return opentracing_propagation_error_code_unknown;
    }

    return opentracing_propagation_error_code_success;
}

static inline opentracing_propagation_error_code
parse_comma_separated_map(jaeger_hashtable* baggage, char* str)
{
    assert(baggage != NULL);
    assert(str != NULL);

    char* comma_context;
    char* kv_str = strtok_r(str, ",", &comma_context);
    while (kv_str != NULL) {
        const opentracing_propagation_error_code result =
            parse_key_value(baggage, kv_str);
        if (result != opentracing_propagation_error_code_success) {
            return result;
        }
        kv_str = strtok_r(NULL, ",", &comma_context);
    }
    return opentracing_propagation_error_code_success;
}

#ifdef __cplusplus
}
#endif

#endif /* JAEGERTRACINGC_STRINGS_H */
