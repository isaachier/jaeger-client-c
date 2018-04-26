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

#include "jaegertracingc/internal/strings.h"
#include "jaegertracingc/propagation.h"

#include "unity.h"

#include "jaegertracingc/options.h"
#include "jaegertracingc/span.h"

typedef struct mock_reader {
    opentracing_text_map_reader base;
} mock_reader;

opentracing_propagation_error_code
mock_reader_foreach_key(opentracing_text_map_reader* reader,
                        opentracing_propagation_error_code (*handler)(
                            void*, const char*, const char*),
                        void* arg)
{
    (void) reader;
    const char* keys[] = {"key%%1", "key%E02", "key3%"};
    const char* values[] = {"value1", "VALUE2", "VaLuE"};
    for (int i = 0, len = sizeof(keys) / sizeof(keys[0]); i < len; i++) {
        opentracing_propagation_error_code return_code =
            handler(arg, keys[i], values[i]);
        if (return_code != opentracing_propagation_error_code_success) {
            return return_code;
        }
    }
    return opentracing_propagation_error_code_success;
}

static inline void test_decode_hex()
{
    TEST_ASSERT_EQUAL(-1, decode_hex('Z'));
    TEST_ASSERT_EQUAL(-1, decode_hex('z'));
    TEST_ASSERT_EQUAL(-1, decode_hex('-'));
    TEST_ASSERT_EQUAL(0xf, decode_hex('F'));
    TEST_ASSERT_EQUAL(0xf, decode_hex('f'));
    TEST_ASSERT_EQUAL(9, decode_hex('9'));
}

static inline void test_decode_uri_value()
{
    const char* encoded[] = {
        "hello%20world", "hello%2", "%", "%f", "%z", "%fz"};
    const char* decoded[] = {"hello world", "hello%2", "%", "%f", "%z", "%fz"};

    for (int i = 0, len = sizeof(encoded) / sizeof(encoded[0]); i < len; i++) {
        char buffer[strlen(encoded[i]) + 1];
        decode_uri_value(buffer, encoded[i]);
        TEST_ASSERT_EQUAL_STRING(decoded[i], buffer);
    }
}

static inline void test_to_lowercase()
{
    const char* uppercase[] = {"HELLO", "WORLD", "test"};
    const char* lowercase[] = {"hello", "world", "test"};

    for (int i = 0, len = sizeof(uppercase) / sizeof(uppercase[0]); i < len;
         i++) {
        char buffer[strlen(uppercase[i]) + 1];
        to_lowercase(buffer, uppercase[i]);
        TEST_ASSERT_EQUAL_STRING(lowercase[i], buffer);
    }
}

static inline void test_extract_from_text_map()
{
    mock_reader reader = {.base = {.foreach_key = &mock_reader_foreach_key}};
    jaeger_span_context* ctx = NULL;
    jaeger_headers_config headers = JAEGERTRACINGC_HEADERS_CONFIG_INIT;
    jaeger_extract_from_text_map(
        (opentracing_text_map_reader*) &reader, &ctx, NULL, &headers);
    TEST_ASSERT_NULL(ctx);
}

void test_propagation()
{
    test_decode_hex();
    test_decode_uri_value();
    test_to_lowercase();
    test_extract_from_text_map();
}
