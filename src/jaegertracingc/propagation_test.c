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

#include "jaegertracingc/propagation.h"

#include "unity.h"

#include "jaegertracingc/hashtable.h"
#include "jaegertracingc/internal/strings.h"
#include "jaegertracingc/internal/test_helpers.h"
#include "jaegertracingc/metrics.h"
#include "jaegertracingc/options.h"
#include "jaegertracingc/span.h"
#include "jaegertracingc/vector.h"

typedef struct mock_text_map_reader {
    opentracing_text_map_reader base;
    const jaeger_vector* key_values;
} mock_text_map_reader;

typedef struct mock_http_headers_reader {
    opentracing_http_headers_reader base;
    const jaeger_vector* key_values;
} mock_http_headers_reader;

typedef struct mock_text_map_writer {
    opentracing_text_map_writer base;
    jaeger_vector* key_values;
} mock_text_map_writer;

typedef struct mock_http_headers_writer {
    opentracing_http_headers_writer base;
    jaeger_vector* key_values;
} mock_http_headers_writer;

static inline opentracing_propagation_error_code
mock_reader_foreach_key(opentracing_text_map_reader* reader,
                        opentracing_propagation_error_code (*handler)(
                            void*, const char*, const char*),
                        void* arg)
{
    mock_text_map_reader* r = (mock_text_map_reader*) reader;
    for (int i = 0, len = jaeger_vector_length(r->key_values); i < len; i++) {
        const jaeger_key_value* kv =
            jaeger_vector_get((jaeger_vector*) r->key_values, i);
        const opentracing_propagation_error_code return_code =
            handler(arg, kv->key, kv->value);
        if (return_code != opentracing_propagation_error_code_success) {
            return return_code;
        }
    }
    return opentracing_propagation_error_code_success;
}

static inline opentracing_propagation_error_code mock_writer_set(
    opentracing_text_map_writer* writer, const char* key, const char* value)
{
    mock_text_map_writer* w = (mock_text_map_writer*) writer;
    jaeger_key_value* kv = jaeger_vector_append(w->key_values);
    TEST_ASSERT_NOT_NULL(kv);
    TEST_ASSERT_TRUE(jaeger_key_value_init(kv, key, value));
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

static inline void test_encode_hex()
{
    TEST_ASSERT_EQUAL('f', encode_hex(0xf));
    TEST_ASSERT_EQUAL('a', encode_hex(0xa));
    TEST_ASSERT_EQUAL('2', encode_hex(2));
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

static inline void test_encode_uri_value()
{
    const char* decoded[] = {"hello world", "hello-world"};
    const char* encoded[] = {"hello%20world", "hello-world"};

    for (int i = 0, len = sizeof(encoded) / sizeof(encoded[0]); i < len; i++) {
        char buffer[strlen(decoded[i]) * 3 + 1];
        encode_uri_value(buffer, decoded[i]);
        TEST_ASSERT_EQUAL_STRING(encoded[i], buffer);
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

static inline void set_up_standard_key_values(jaeger_vector* key_values)
{
    JAEGERTRACINGC_VECTOR_FOR_EACH(
        key_values, jaeger_key_value_destroy, jaeger_key_value);
    jaeger_vector_clear(key_values);
    const char* keys[] = {"key%%1",
                          "key%E02",
                          "key3%",
                          JAEGERTRACINGC_TRACE_CONTEXT_HEADER_NAME,
                          JAEGERTRACINGC_TRACE_BAGGAGE_HEADER_PREFIX "k1",
                          JAEGERTRACINGC_TRACE_BAGGAGE_HEADER_PREFIX "k2"};
    const char* values[] = {
        "value1", "VALUE2", "VaLuE", "ab:cd:0:0", "v1", "v2"};
    for (int i = 0, len = sizeof(keys) / sizeof(keys[0]); i < len; i++) {
        jaeger_key_value* kv = jaeger_vector_append(key_values);
        TEST_ASSERT_NOT_NULL(kv);
        TEST_ASSERT_TRUE(jaeger_key_value_init(kv, keys[i], values[i]));
        TEST_ASSERT_EQUAL(i + 1, jaeger_vector_length(key_values));
    }
}

static inline void set_up_alternative_baggage_format(jaeger_vector* key_values)
{
    JAEGERTRACINGC_VECTOR_FOR_EACH(
        key_values, jaeger_key_value_destroy, jaeger_key_value);
    jaeger_vector_clear(key_values);
    const char* keys[] = {"key%%1",
                          "key%E02",
                          "key3%",
                          JAEGERTRACINGC_TRACE_CONTEXT_HEADER_NAME,
                          JAEGERTRACINGC_BAGGAGE_HEADER};
    const char* values[] = {
        "value1", "VALUE2", "VaLuE", "ab:cd:0:0", "k3=value3,key-4=value-x"};
    for (int i = 0, len = sizeof(keys) / sizeof(keys[0]); i < len; i++) {
        jaeger_key_value* kv = jaeger_vector_append(key_values);
        TEST_ASSERT_NOT_NULL(kv);
        TEST_ASSERT_TRUE(jaeger_key_value_init(kv, keys[i], values[i]));
        TEST_ASSERT_EQUAL(i + 1, jaeger_vector_length(key_values));
    }
}

static inline void set_up_debug_header(jaeger_vector* key_values)
{
    JAEGERTRACINGC_VECTOR_FOR_EACH(
        key_values, jaeger_key_value_destroy, jaeger_key_value);
    jaeger_vector_clear(key_values);
    const char* keys[] = {"key%%1",
                          "key%E02",
                          "key3%",
                          JAEGERTRACINGC_TRACE_CONTEXT_HEADER_NAME,
                          JAEGERTRACINGC_DEBUG_HEADER};
    const char* values[] = {
        "value1", "VALUE2", "VaLuE", "ab:cd:0:0", "test-debug-header"};
    for (int i = 0, len = sizeof(keys) / sizeof(keys[0]); i < len; i++) {
        jaeger_key_value* kv = jaeger_vector_append(key_values);
        TEST_ASSERT_NOT_NULL(kv);
        TEST_ASSERT_TRUE(jaeger_key_value_init(kv, keys[i], values[i]));
        TEST_ASSERT_EQUAL(i + 1, jaeger_vector_length(key_values));
    }
}

static inline void set_up_corrupt_baggage(jaeger_vector* key_values)
{
    JAEGERTRACINGC_VECTOR_FOR_EACH(
        key_values, jaeger_key_value_destroy, jaeger_key_value);
    jaeger_vector_clear(key_values);
    const char* keys[] = {"key%%1",
                          "key%E02",
                          "key3%",
                          JAEGERTRACINGC_TRACE_CONTEXT_HEADER_NAME,
                          JAEGERTRACINGC_BAGGAGE_HEADER};
    const char* values[] = {
        "value1", "VALUE2", "VaLuE", "ab:cd:0:0", "key-no-value"};
    for (int i = 0, len = sizeof(keys) / sizeof(keys[0]); i < len; i++) {
        jaeger_key_value* kv = jaeger_vector_append(key_values);
        TEST_ASSERT_NOT_NULL(kv);
        TEST_ASSERT_TRUE(jaeger_key_value_init(kv, keys[i], values[i]));
        TEST_ASSERT_EQUAL(i + 1, jaeger_vector_length(key_values));
    }
}

static inline void set_up_empty_headers(jaeger_vector* key_values)
{
    JAEGERTRACINGC_VECTOR_FOR_EACH(
        key_values, jaeger_key_value_destroy, jaeger_key_value);
    jaeger_vector_clear(key_values);
}

static inline void set_up_decode_failure(jaeger_vector* key_values)
{
    JAEGERTRACINGC_VECTOR_FOR_EACH(
        key_values, jaeger_key_value_destroy, jaeger_key_value);
    jaeger_vector_clear(key_values);
    const char* keys[] = {
        "key%%1", "key%E02", "key3%", JAEGERTRACINGC_TRACE_CONTEXT_HEADER_NAME};
    const char* values[] = {"value1", "VALUE2", "VaLuE", "xy:z-:0:0"};
    for (int i = 0, len = sizeof(keys) / sizeof(keys[0]); i < len; i++) {
        jaeger_key_value* kv = jaeger_vector_append(key_values);
        TEST_ASSERT_NOT_NULL(kv);
        TEST_ASSERT_TRUE(jaeger_key_value_init(kv, keys[i], values[i]));
        TEST_ASSERT_EQUAL(i + 1, jaeger_vector_length(key_values));
    }
}

static inline void test_text_map()
{
    jaeger_metrics metrics;
    jaeger_default_metrics_init(&metrics);

    /* Test standard key-value pairs. */
    jaeger_vector key_values;
    TEST_ASSERT_TRUE(jaeger_vector_init(&key_values, sizeof(jaeger_key_value)));
    set_up_standard_key_values(&key_values);
    mock_text_map_reader reader = {
        .base = {.foreach_key = &mock_reader_foreach_key},
        .key_values = &key_values};
    jaeger_span_context* ctx;
    jaeger_headers_config headers = JAEGERTRACINGC_HEADERS_CONFIG_INIT;
    TEST_ASSERT_EQUAL(
        opentracing_propagation_error_code_success,
        jaeger_extract_from_text_map(
            (opentracing_text_map_reader*) &reader, &ctx, &metrics, &headers));
    TEST_ASSERT_NOT_NULL(ctx);
    TEST_ASSERT_EQUAL(0xab, ctx->trace_id.low);
    TEST_ASSERT_EQUAL(0xcd, ctx->span_id);
    TEST_ASSERT_EQUAL(0, ctx->parent_id);
    TEST_ASSERT_EQUAL(0, ctx->flags);
    TEST_ASSERT_EQUAL(2, ctx->baggage.size);
    const jaeger_key_value* kv = jaeger_hashtable_find(&ctx->baggage, "k1");
    TEST_ASSERT_NOT_NULL(kv);
    TEST_ASSERT_EQUAL_STRING("k1", kv->key);
    TEST_ASSERT_EQUAL_STRING("v1", kv->value);
    kv = jaeger_hashtable_find(&ctx->baggage, "k2");
    TEST_ASSERT_NOT_NULL(kv);
    TEST_ASSERT_EQUAL_STRING("k2", kv->key);
    TEST_ASSERT_EQUAL_STRING("v2", kv->value);

    /* Test encode/decode cycle gets original object. */
    JAEGERTRACINGC_VECTOR_FOR_EACH(
        &key_values, jaeger_key_value_destroy, jaeger_key_value);
    jaeger_vector_clear(&key_values);
    ctx->trace_id.high = 0;
    ctx->trace_id.low = 0xab;
    ctx->span_id = 0xcd;
    ctx->parent_id = 0xef;
    ctx->flags = jaeger_sampling_flag_sampled;
    jaeger_hashtable_clear(&ctx->baggage);
    TEST_ASSERT_TRUE(jaeger_hashtable_put(&ctx->baggage, "k1", "v1"));
    TEST_ASSERT_EQUAL(1, ctx->baggage.size);
    mock_text_map_writer writer = {.base = {.set = &mock_writer_set},
                                   .key_values = &key_values};
    const jaeger_headers_config config = JAEGERTRACINGC_HEADERS_CONFIG_INIT;
    TEST_ASSERT_EQUAL(
        opentracing_propagation_error_code_success,
        jaeger_inject_into_text_map(
            (opentracing_text_map_writer*) &writer, ctx, &config));
    jaeger_span_context* ctx_copy;
    TEST_ASSERT_EQUAL(
        opentracing_propagation_error_code_success,
        jaeger_extract_from_text_map((opentracing_text_map_reader*) &reader,
                                     &ctx_copy,
                                     &metrics,
                                     &config));
    TEST_ASSERT_EQUAL(ctx->trace_id.high, ctx_copy->trace_id.high);
    TEST_ASSERT_EQUAL(ctx->trace_id.low, ctx_copy->trace_id.low);
    TEST_ASSERT_EQUAL(ctx->span_id, ctx_copy->span_id);
    TEST_ASSERT_EQUAL(ctx->parent_id, ctx_copy->parent_id);
    TEST_ASSERT_EQUAL(ctx->flags, ctx_copy->flags);
    TEST_ASSERT_EQUAL(ctx->baggage.size, ctx_copy->baggage.size);
    kv = jaeger_hashtable_find(&ctx->baggage, "k1");
    TEST_ASSERT_NOT_NULL(kv);
    const jaeger_key_value* kv_copy =
        jaeger_hashtable_find(&ctx_copy->baggage, "k1");
    TEST_ASSERT_NOT_NULL(kv_copy);
    TEST_ASSERT_EQUAL_STRING(kv->value, kv_copy->value);

    ((jaeger_destructible*) ctx)->destroy((jaeger_destructible*) ctx);
    ((jaeger_destructible*) ctx_copy)->destroy((jaeger_destructible*) ctx_copy);
    jaeger_free(ctx);
    jaeger_free(ctx_copy);
    JAEGERTRACINGC_VECTOR_FOR_EACH(
        &key_values, jaeger_key_value_destroy, jaeger_key_value);
    jaeger_vector_destroy(&key_values);
    jaeger_metrics_destroy(&metrics);
}

static inline void test_http_headers()
{
    jaeger_metrics metrics;
    jaeger_default_metrics_init(&metrics);

    /* Test standard headers. */
    jaeger_vector key_values;
    TEST_ASSERT_TRUE(jaeger_vector_init(&key_values, sizeof(jaeger_key_value)));
    set_up_standard_key_values(&key_values);
    mock_http_headers_reader reader = {
        .base = {.base = {.foreach_key = &mock_reader_foreach_key}},
        .key_values = &key_values};
    jaeger_span_context* ctx = NULL;
    jaeger_headers_config headers = JAEGERTRACINGC_HEADERS_CONFIG_INIT;
    TEST_ASSERT_EQUAL(opentracing_propagation_error_code_success,
                      jaeger_extract_from_http_headers(
                          (opentracing_http_headers_reader*) &reader,
                          &ctx,
                          &metrics,
                          &headers));
    TEST_ASSERT_NOT_NULL(ctx);
    TEST_ASSERT_EQUAL(0xab, ctx->trace_id.low);
    TEST_ASSERT_EQUAL(0xcd, ctx->span_id);
    TEST_ASSERT_EQUAL(0, ctx->parent_id);
    TEST_ASSERT_EQUAL(0, ctx->flags);
    TEST_ASSERT_EQUAL(2, ctx->baggage.size);
    const jaeger_key_value* kv = jaeger_hashtable_find(&ctx->baggage, "k1");
    TEST_ASSERT_NOT_NULL(kv);
    TEST_ASSERT_EQUAL_STRING("k1", kv->key);
    TEST_ASSERT_EQUAL_STRING("v1", kv->value);
    kv = jaeger_hashtable_find(&ctx->baggage, "k2");
    TEST_ASSERT_NOT_NULL(kv);
    TEST_ASSERT_EQUAL_STRING("k2", kv->key);
    TEST_ASSERT_EQUAL_STRING("v2", kv->value);
    ((jaeger_destructible*) ctx)->destroy((jaeger_destructible*) ctx);
    jaeger_free(ctx);

    /* Test alternative baggage format. */
    set_up_alternative_baggage_format(&key_values);
    TEST_ASSERT_EQUAL(opentracing_propagation_error_code_success,
                      jaeger_extract_from_http_headers(
                          (opentracing_http_headers_reader*) &reader,
                          &ctx,
                          &metrics,
                          &headers));
    TEST_ASSERT_NOT_NULL(ctx);
    TEST_ASSERT_EQUAL(0xab, ctx->trace_id.low);
    TEST_ASSERT_EQUAL(0xcd, ctx->span_id);
    TEST_ASSERT_EQUAL(0, ctx->parent_id);
    TEST_ASSERT_EQUAL(0, ctx->flags);
    TEST_ASSERT_EQUAL(2, ctx->baggage.size);
    kv = jaeger_hashtable_find(&ctx->baggage, "k3");
    TEST_ASSERT_NOT_NULL(kv);
    TEST_ASSERT_EQUAL_STRING("k3", kv->key);
    TEST_ASSERT_EQUAL_STRING("value3", kv->value);
    kv = jaeger_hashtable_find(&ctx->baggage, "key-4");
    TEST_ASSERT_NOT_NULL(kv);
    TEST_ASSERT_EQUAL_STRING("key-4", kv->key);
    TEST_ASSERT_EQUAL_STRING("value-x", kv->value);
    ((jaeger_destructible*) ctx)->destroy((jaeger_destructible*) ctx);
    jaeger_free(ctx);

    /* Test debug header. */
    set_up_debug_header(&key_values);
    TEST_ASSERT_EQUAL(opentracing_propagation_error_code_success,
                      jaeger_extract_from_http_headers(
                          (opentracing_http_headers_reader*) &reader,
                          &ctx,
                          &metrics,
                          &headers));
    TEST_ASSERT_NOT_NULL(ctx);
    TEST_ASSERT_EQUAL(0xab, ctx->trace_id.low);
    TEST_ASSERT_EQUAL(0xcd, ctx->span_id);
    TEST_ASSERT_EQUAL(0, ctx->parent_id);
    TEST_ASSERT_EQUAL(jaeger_sampling_flag_debug | jaeger_sampling_flag_sampled,
                      ctx->flags);
    TEST_ASSERT_EQUAL_STRING(ctx->debug_id, "test-debug-header");
    ((jaeger_destructible*) ctx)->destroy((jaeger_destructible*) ctx);
    jaeger_free(ctx);

    /* Test corrupt baggage. */
    set_up_corrupt_baggage(&key_values);
    TEST_ASSERT_EQUAL(opentracing_propagation_error_code_span_context_corrupted,
                      jaeger_extract_from_http_headers(
                          (opentracing_http_headers_reader*) &reader,
                          &ctx,
                          &metrics,
                          &headers));
    TEST_ASSERT_NULL(ctx);

    /* Test empty headers. */
    set_up_empty_headers(&key_values);
    TEST_ASSERT_EQUAL(opentracing_propagation_error_code_success,
                      jaeger_extract_from_http_headers(
                          (opentracing_http_headers_reader*) &reader,
                          &ctx,
                          &metrics,
                          &headers));
    TEST_ASSERT_NULL(ctx);

    /* Test decode failure. */
    set_up_decode_failure(&key_values);
    TEST_ASSERT_EQUAL(opentracing_propagation_error_code_span_context_corrupted,
                      jaeger_extract_from_http_headers(
                          (opentracing_http_headers_reader*) &reader,
                          &ctx,
                          &metrics,
                          &headers));
    TEST_ASSERT_NULL(ctx);

    JAEGERTRACINGC_VECTOR_FOR_EACH(
        &key_values, jaeger_key_value_destroy, jaeger_key_value);
    jaeger_vector_destroy(&key_values);
    jaeger_metrics_destroy(&metrics);
}

static inline int
binary_writer_callback(void* arg, const char* data, size_t len)
{
    jaeger_vector* buffer = (jaeger_vector*) arg;
    char* addr =
        (char*) jaeger_vector_extend(buffer, jaeger_vector_length(buffer), len);
    TEST_ASSERT_NOT_NULL(addr);
    memcpy(addr, data, len);
    return len;
}

static inline int binary_reader_callback(void* arg, char* data, size_t len)
{
    jaeger_vector* buffer = (jaeger_vector*) arg;
    TEST_ASSERT_GREATER_OR_EQUAL_INT(len, jaeger_vector_length(buffer));
    for (int i = 0; i < (int) len; i++) {
        data[i] = *(char*) jaeger_vector_get(buffer, 0);
        jaeger_vector_remove(buffer, 0);
    }
    return len;
}

static inline void test_binary()
{
    jaeger_metrics metrics;
    jaeger_default_metrics_init(&metrics);

    jaeger_span_context ctx;
    TEST_ASSERT_TRUE(jaeger_span_context_init(&ctx));
    ctx.trace_id = (jaeger_trace_id){.high = 0x1234, .low = 0x5678};
    ctx.parent_id = 0xEF00;
    ctx.span_id = 0xABCD;
    enum {
        max_baggage_items = 15,
        min_baggage_items = 1,
        max_baggage_str_len = 20
    };
    const int num_baggage_items =
        rand() % (max_baggage_items - min_baggage_items) + min_baggage_items;
    for (int i = 0; i < num_baggage_items; i++) {
        char key[rand() % max_baggage_str_len + 1];
        random_string(key, sizeof(key));
        char value[rand() % max_baggage_str_len + 1];
        random_string(value, sizeof(value));
        TEST_ASSERT_TRUE(jaeger_hashtable_put(&ctx.baggage, key, value));
    }

    jaeger_vector binary_buffer;
    TEST_ASSERT_TRUE(jaeger_vector_init(&binary_buffer, 1));
    TEST_ASSERT_EQUAL(opentracing_propagation_error_code_success,
                      jaeger_inject_into_binary(&binary_writer_callback,
                                                (void*) &binary_buffer,
                                                &ctx));
    jaeger_span_context* ctx_copy;
    TEST_ASSERT_EQUAL(
        opentracing_propagation_error_code_success,
        jaeger_extract_from_binary(
            &binary_reader_callback, &binary_buffer, &ctx_copy, &metrics));
    TEST_ASSERT_EQUAL(ctx.trace_id.high, ctx_copy->trace_id.high);
    TEST_ASSERT_EQUAL(ctx.trace_id.low, ctx_copy->trace_id.low);
    TEST_ASSERT_EQUAL(ctx.span_id, ctx_copy->span_id);
    TEST_ASSERT_EQUAL(ctx.parent_id, ctx_copy->parent_id);
    TEST_ASSERT_EQUAL(ctx.baggage.size, ctx_copy->baggage.size);

    jaeger_span_context_destroy((jaeger_destructible*) &ctx);
    jaeger_span_context_destroy((jaeger_destructible*) ctx_copy);
    jaeger_free(ctx_copy);
    jaeger_vector_destroy(&binary_buffer);
    jaeger_metrics_destroy(&metrics);
}

static inline void test_parse_key_value()
{
    jaeger_hashtable baggage;
    TEST_ASSERT_TRUE(jaeger_hashtable_init(&baggage));
    TEST_ASSERT_EQUAL(opentracing_propagation_error_code_span_context_corrupted,
                      parse_key_value(&baggage, ""));
    jaeger_hashtable_destroy(&baggage);
}

void test_propagation()
{
    test_decode_hex();
    test_encode_hex();
    test_decode_uri_value();
    test_encode_uri_value();
    test_to_lowercase();
    test_text_map();
    test_http_headers();
    test_binary();
    test_parse_key_value();
}
