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
 * Trace ID representation.
 */

#include "jaegertracingc/propagation.h"

#include <ctype.h>

#include "jaegertracingc/metrics.h"
#include "jaegertracingc/options.h"
#include "jaegertracingc/span.h"

static inline int decode_hex(char ch)
{
    if (islower(ch)) {
        return (ch - 'a') + 10;
    }
    if (isupper(ch)) {
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

typedef struct extract_text_map_arg {
    jaeger_span_context* ctx;
    jaeger_metrics* metrics;
    const jaeger_headers_config* config;
    void (*normalize_key)(char* restrict, const char* restrict);
    void (*decode_value)(char* restrict, const char* restrict);
} extract_text_map_arg;

static opentracing_propagation_error_code
extract_text_map_callback(void* arg, const char* key, const char* value)
{
    jaeger_span_context* ctx = ((extract_text_map_arg*) arg)->ctx;
    const jaeger_headers_config* config = ((extract_text_map_arg*) arg)->config;
    void (*normalize_key)(char*, const char*) =
        ((extract_text_map_arg*) arg)->normalize_key;
    void (*decode_value)(char*, const char*) =
        ((extract_text_map_arg*) arg)->decode_value;
    char key_buffer[strlen(key) + 1];

    normalize_key(key_buffer, key);
    assert(ctx != NULL);
    if (strcmp(key_buffer, config->trace_context_header_name) == 0) {
        char value_buffer[strlen(value) + 1];
        decode_value(value_buffer, value);
        if (!jaeger_span_context_scan(ctx, value_buffer)) {
            return opentracing_propagation_error_code_span_context_corrupted;
        }
    }
    jaeger_key_value* kv = jaeger_vector_append(&ctx->baggage);
    if (kv == NULL) {
        return opentracing_propagation_error_code_unknown;
    }
    if (!jaeger_key_value_init(kv, key_buffer, value)) {
        ctx->baggage.len--;
        return opentracing_propagation_error_code_unknown;
    }
    return opentracing_propagation_error_code_success;
}

static inline opentracing_propagation_error_code
extract_from_text_map_helper(opentracing_text_map_reader* reader,
                             extract_text_map_arg* arg)
{
    arg->ctx = jaeger_malloc(sizeof(*arg->ctx));
    if (arg->ctx == NULL || !jaeger_span_context_init(arg->ctx)) {
        goto cleanup;
    }
    reader->foreach_key(reader, &extract_text_map_callback, arg);

cleanup:
    jaeger_span_context_destroy((jaeger_destructible*) arg->ctx);
    jaeger_free(arg->ctx);
    arg->ctx = NULL;
    return opentracing_propagation_error_code_unknown;
}

opentracing_propagation_error_code
jaeger_extract_from_text_map(opentracing_text_map_reader* reader,
                             jaeger_span_context** ctx,
                             jaeger_metrics* metrics,
                             const jaeger_headers_config* config)
{
    assert(ctx != NULL);
    return extract_from_text_map_helper(
        reader,
        &(extract_text_map_arg){.ctx = *ctx,
                                .config = config,
                                .normalize_key = &copy_str,
                                .decode_value = &copy_str});
}

opentracing_propagation_error_code
jaeger_extract_from_http_headers(opentracing_http_headers_reader* reader,
                                 jaeger_span_context** ctx,
                                 jaeger_metrics* metrics,
                                 const jaeger_headers_config* config)
{
    assert(ctx != NULL);
    return extract_from_text_map_helper(
        (opentracing_text_map_reader*) reader,
        &(extract_text_map_arg){.ctx = *ctx,
                                .config = config,
                                .normalize_key = &to_lowercase,
                                .decode_value = &decode_uri_value});
}

opentracing_propagation_error_code
jaeger_extract_from_binary(int (*callback)(void*, char*, size_t),
                           void* arg,
                           jaeger_span_context** ctx,
                           jaeger_metrics* metrics,
                           const jaeger_headers_config* config);

opentracing_propagation_error_code
jaeger_extract_from_custom(opentracing_custom_carrier_reader* reader,
                           jaeger_span_context** ctx,
                           jaeger_metrics* metrics,
                           const jaeger_headers_config* config);

opentracing_propagation_error_code
jaeger_inject_into_text_map(opentracing_text_map_writer* writer,
                            const jaeger_span_context* ctx,
                            jaeger_metrics* metrics,
                            const jaeger_headers_config* config);

opentracing_propagation_error_code
jaeger_inject_into_http_headers(opentracing_http_headers_writer* writer,
                                const jaeger_span_context* ctx,
                                jaeger_metrics* metrics,
                                const jaeger_headers_config* config);

opentracing_propagation_error_code
jaeger_inject_into_binary(opentracing_http_headers_writer* writer,
                          int (*callback)(void*, const char*, size_t),
                          jaeger_metrics* metrics,
                          const jaeger_headers_config* config);

opentracing_propagation_error_code
jaeger_inject_into_custom(opentracing_custom_carrier_writer* writer,
                          const jaeger_span_context* ctx,
                          jaeger_metrics* metrics,
                          const jaeger_headers_config* config);
