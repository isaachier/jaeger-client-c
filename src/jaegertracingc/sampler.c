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

#include "jaegertracingc/sampler.h"

#include <arpa/inet.h>
#include <errno.h>
#include <jansson.h>

#include "jaegertracingc/random.h"

#define HTTP_OK 200
#define SAMPLER_GROWTH_FACTOR 2
#define DEFAULT_MAX_OPERATIONS 2000
#define DEFAULT_SAMPLING_RATE 0.001

static bool jaeger_const_sampler_is_sampled(jaeger_sampler* sampler,
                                            const jaeger_trace_id* trace_id,
                                            const char* operation_name,
                                            jaeger_vector* tags)
{
    (void) trace_id;
    (void) operation_name;
    jaeger_const_sampler* s = (jaeger_const_sampler*) sampler;
    if (tags != NULL &&
        jaeger_vector_reserve(tags, jaeger_vector_length(tags) + 2)) {
        jaeger_tag tag = JAEGERTRACINGC_TAG_INIT;
        tag.key = JAEGERTRACINGC_SAMPLER_TYPE_TAG_KEY;
        tag.value_case = JAEGERTRACING__PROTOBUF__TAG__VALUE_STR_VALUE;
        tag.str_value = JAEGERTRACINGC_SAMPLER_TYPE_CONST;
        if (!jaeger_tag_vector_append(tags, &tag)) {
            return s->decision;
        }

        tag.key = JAEGERTRACINGC_SAMPLER_PARAM_TAG_KEY;
        tag.value_case = JAEGERTRACING__PROTOBUF__TAG__VALUE_BOOL_VALUE;
        tag.bool_value = s->decision;
        jaeger_tag_vector_append(tags, &tag);
    }
    return s->decision;
}

static void jaeger_sampler_noop_destroy(jaeger_destructible* destructible)
{
    (void) destructible;
}

void jaeger_const_sampler_init(jaeger_const_sampler* sampler, bool decision)
{
    assert(sampler != NULL);
    sampler->decision = decision;
    ((jaeger_sampler*) sampler)->is_sampled = &jaeger_const_sampler_is_sampled;
    ((jaeger_destructible*) sampler)->destroy = &jaeger_sampler_noop_destroy;
}

static bool
jaeger_probabilistic_sampler_is_sampled(jaeger_sampler* sampler,
                                        const jaeger_trace_id* trace_id,
                                        const char* operation_name,
                                        jaeger_vector* tags)
{
    (void) trace_id;
    (void) operation_name;
    jaeger_probabilistic_sampler* s = (jaeger_probabilistic_sampler*) sampler;
    const double random_value = ((double) ((uint64_t) random64())) / UINT64_MAX;
    const bool decision = (s->sampling_rate >= random_value);
    if (tags != NULL &&
        jaeger_vector_reserve(tags, jaeger_vector_length(tags) + 2)) {
        jaeger_tag tag = JAEGERTRACINGC_TAG_INIT;
        tag.key = JAEGERTRACINGC_SAMPLER_TYPE_TAG_KEY;
        tag.value_case = JAEGERTRACING__PROTOBUF__TAG__VALUE_STR_VALUE;
        tag.str_value = JAEGERTRACINGC_SAMPLER_TYPE_PROBABILISTIC;
        if (!jaeger_tag_vector_append(tags, &tag)) {
            return decision;
        }

        tag.key = JAEGERTRACINGC_SAMPLER_PARAM_TAG_KEY;
        tag.value_case = JAEGERTRACING__PROTOBUF__TAG__VALUE_DOUBLE_VALUE;
        tag.double_value = s->sampling_rate;
        jaeger_tag_vector_append(tags, &tag);
    }
    return decision;
}

void jaeger_probabilistic_sampler_init(jaeger_probabilistic_sampler* sampler,
                                       double sampling_rate)
{
    assert(sampler != NULL);
    ((jaeger_sampler*) sampler)->is_sampled =
        &jaeger_probabilistic_sampler_is_sampled;
    ((jaeger_destructible*) sampler)->destroy = &jaeger_sampler_noop_destroy;
    sampler->sampling_rate = JAEGERTRACINGC_CLAMP(sampling_rate, 0, 1);
    jaeger_duration duration;
    jaeger_duration_now(&duration);
}

static bool
jaeger_rate_limiting_sampler_is_sampled(jaeger_sampler* sampler,
                                        const jaeger_trace_id* trace_id,
                                        const char* operation_name,
                                        jaeger_vector* tags)
{
    (void) trace_id;
    (void) operation_name;
    assert(sampler != NULL);
    jaeger_rate_limiting_sampler* s = (jaeger_rate_limiting_sampler*) sampler;
    const bool decision = jaeger_token_bucket_check_credit(&s->tok, 1);
    if (tags != NULL &&
        jaeger_vector_reserve(tags, jaeger_vector_length(tags) + 2)) {
        jaeger_tag tag = JAEGERTRACINGC_TAG_INIT;
        tag.key = JAEGERTRACINGC_SAMPLER_TYPE_TAG_KEY;
        tag.value_case = JAEGERTRACING__PROTOBUF__TAG__VALUE_STR_VALUE;
        tag.str_value = JAEGERTRACINGC_SAMPLER_TYPE_RATE_LIMITING;
        jaeger_tag_vector_append(tags, &tag);

        tag.key = JAEGERTRACINGC_SAMPLER_PARAM_TAG_KEY;
        tag.value_case = JAEGERTRACING__PROTOBUF__TAG__VALUE_DOUBLE_VALUE;
        tag.double_value = s->max_traces_per_second;
        jaeger_tag_vector_append(tags, &tag);
    }
    return decision;
}

void jaeger_rate_limiting_sampler_init(jaeger_rate_limiting_sampler* sampler,
                                       double max_traces_per_second)
{
    assert(sampler != NULL);
    ((jaeger_sampler*) sampler)->is_sampled =
        jaeger_rate_limiting_sampler_is_sampled;
    ((jaeger_destructible*) sampler)->destroy = &jaeger_sampler_noop_destroy;
    jaeger_token_bucket_init(&sampler->tok,
                             max_traces_per_second,
                             JAEGERTRACINGC_MAX(max_traces_per_second, 1));
    sampler->max_traces_per_second = max_traces_per_second;
}

static bool jaeger_guaranteed_throughput_probabilistic_sampler_is_sampled(
    jaeger_sampler* sampler,
    const jaeger_trace_id* trace_id,
    const char* operation_name,
    jaeger_vector* tags)
{
    (void) trace_id;
    (void) operation_name;
    assert(sampler != NULL);
    jaeger_guaranteed_throughput_probabilistic_sampler* s =
        (jaeger_guaranteed_throughput_probabilistic_sampler*) sampler;
    bool decision =
        ((jaeger_sampler*) &s->probabilistic_sampler)
            ->is_sampled((jaeger_sampler*) &s->probabilistic_sampler,
                         trace_id,
                         operation_name,
                         NULL);
    if (decision) {
        ((jaeger_sampler*) &s->lower_bound_sampler)
            ->is_sampled((jaeger_sampler*) &s->lower_bound_sampler,
                         trace_id,
                         operation_name,
                         NULL);
        if (tags != NULL &&
            jaeger_vector_reserve(tags, jaeger_vector_length(tags) + 2)) {
            jaeger_tag tag = JAEGERTRACINGC_TAG_INIT;
            tag.key = JAEGERTRACINGC_SAMPLER_TYPE_TAG_KEY;
            tag.value_case = JAEGERTRACING__PROTOBUF__TAG__VALUE_STR_VALUE;
            tag.str_value = JAEGERTRACINGC_SAMPLER_TYPE_PROBABILISTIC;
            jaeger_tag_vector_append(tags, &tag);

            tag.key = JAEGERTRACINGC_SAMPLER_PARAM_TAG_KEY;
            tag.value_case = JAEGERTRACING__PROTOBUF__TAG__VALUE_DOUBLE_VALUE;
            tag.double_value = s->probabilistic_sampler.sampling_rate;
            jaeger_tag_vector_append(tags, &tag);
        }
        return true;
    }
    decision = ((jaeger_sampler*) &s->lower_bound_sampler)
                   ->is_sampled((jaeger_sampler*) &s->lower_bound_sampler,
                                trace_id,
                                operation_name,
                                NULL);
    if (tags != NULL &&
        jaeger_vector_reserve(tags, jaeger_vector_length(tags) + 2)) {
        jaeger_tag tag = JAEGERTRACINGC_TAG_INIT;
        tag.key = JAEGERTRACINGC_SAMPLER_TYPE_TAG_KEY;
        tag.value_case = JAEGERTRACING__PROTOBUF__TAG__VALUE_STR_VALUE;
        tag.str_value = JAEGERTRACINGC_SAMPLER_TYPE_LOWER_BOUND;
        jaeger_tag_vector_append(tags, &tag);

        tag.key = JAEGERTRACINGC_SAMPLER_PARAM_TAG_KEY;
        tag.value_case = JAEGERTRACING__PROTOBUF__TAG__VALUE_DOUBLE_VALUE;
        tag.double_value = s->lower_bound_sampler.max_traces_per_second;
        jaeger_tag_vector_append(tags, &tag);
    }
    return decision;
}

static void jaeger_guaranteed_throughput_probabilistic_sampler_destroy(
    jaeger_destructible* sampler)
{
    assert(sampler != NULL);
    jaeger_guaranteed_throughput_probabilistic_sampler* s =
        (jaeger_guaranteed_throughput_probabilistic_sampler*) sampler;
    ((jaeger_destructible*) &s->probabilistic_sampler)
        ->destroy((jaeger_destructible*) &s->probabilistic_sampler);
    ((jaeger_destructible*) &s->lower_bound_sampler)
        ->destroy((jaeger_destructible*) &s->lower_bound_sampler);
}

void jaeger_guaranteed_throughput_probabilistic_sampler_init(
    jaeger_guaranteed_throughput_probabilistic_sampler* sampler,
    double lower_bound,
    double sampling_rate)
{
    assert(sampler != NULL);
    ((jaeger_sampler*) sampler)->is_sampled =
        &jaeger_guaranteed_throughput_probabilistic_sampler_is_sampled;
    ((jaeger_destructible*) sampler)->destroy =
        &jaeger_guaranteed_throughput_probabilistic_sampler_destroy;
    jaeger_probabilistic_sampler_init(&sampler->probabilistic_sampler,
                                      sampling_rate);
    jaeger_rate_limiting_sampler_init(&sampler->lower_bound_sampler,
                                      lower_bound);
}

void jaeger_guaranteed_throughput_probabilistic_sampler_update(
    jaeger_guaranteed_throughput_probabilistic_sampler* sampler,
    double lower_bound,
    double sampling_rate)
{
    assert(sampler != NULL);
    if (sampler->probabilistic_sampler.sampling_rate != sampling_rate) {
        ((jaeger_destructible*) &sampler->probabilistic_sampler)
            ->destroy((jaeger_destructible*) &sampler->probabilistic_sampler);
        jaeger_probabilistic_sampler_init(&sampler->probabilistic_sampler,
                                          sampling_rate);
    }
    if (sampler->lower_bound_sampler.max_traces_per_second != lower_bound) {
        ((jaeger_destructible*) &sampler->lower_bound_sampler)
            ->destroy((jaeger_destructible*) &sampler->lower_bound_sampler);
        jaeger_rate_limiting_sampler_init(&sampler->lower_bound_sampler,
                                          lower_bound);
    }
}

static int op_name_cmp(const void* lhs, const void* rhs)
{
    assert(lhs != NULL);
    assert(rhs != NULL);
    const jaeger_operation_sampler* lhs_op_sampler =
        (const jaeger_operation_sampler*) lhs;
    const jaeger_operation_sampler* rhs_op_sampler =
        (const jaeger_operation_sampler*) rhs;
    return strcmp(lhs_op_sampler->operation_name,
                  rhs_op_sampler->operation_name);
}

static inline bool
samplers_from_strategies(const jaeger_per_operation_strategy* strategies,
                         jaeger_vector* vec)
{
    assert(strategies != NULL);
    assert(vec != NULL);
    jaeger_vector_clear(vec);
    if (!jaeger_vector_reserve(vec, strategies->n_per_operation_strategy)) {
        return false;
    }

    bool success = true;
    int index = 0;
    for (int i = 0; i < (int) strategies->n_per_operation_strategy; i++) {
        const jaeger_operation_strategy* strategy =
            strategies->per_operation_strategy[i];
        if (strategy == NULL || strategy->probabilistic == NULL) {
            continue;
        }
        jaeger_operation_sampler* op_sampler = jaeger_vector_append(vec);
        op_sampler->operation_name = jaeger_strdup(strategy->operation);
        if (op_sampler->operation_name == NULL) {
            success = false;
            break;
        }

        jaeger_guaranteed_throughput_probabilistic_sampler_init(
            &op_sampler->sampler,
            strategies->default_lower_bound_traces_per_second,
            strategy->probabilistic->sampling_rate);
        index++;
    }

    if (!success) {
        for (int i = 0; i < index; i++) {
            jaeger_operation_sampler_destroy(jaeger_vector_get(vec, index));
        }
        jaeger_vector_clear(vec);
        return false;
    }

    jaeger_vector_sort(vec, &op_name_cmp);
    return true;
}

static inline jaeger_operation_sampler*
jaeger_adaptive_sampler_find_sampler(jaeger_adaptive_sampler* sampler,
                                     const char* operation_name)
{
    char operation_name_copy[strlen(operation_name) + 1];
    strncpy(operation_name_copy, operation_name, sizeof(operation_name_copy));
    const jaeger_operation_sampler key = {.operation_name =
                                              operation_name_copy};
    return jaeger_vector_bsearch(&sampler->op_samplers, &key, &op_name_cmp);
}

static bool jaeger_adaptive_sampler_is_sampled(jaeger_sampler* sampler,
                                               const jaeger_trace_id* trace_id,
                                               const char* operation_name,
                                               jaeger_vector* tags)
{
    (void) trace_id;
    (void) operation_name;
    assert(sampler != NULL);
    jaeger_adaptive_sampler* s = (jaeger_adaptive_sampler*) sampler;
    jaeger_mutex_lock(&s->mutex);
    jaeger_operation_sampler* op_sampler =
        jaeger_adaptive_sampler_find_sampler(s, operation_name);
    if (op_sampler != NULL) {
        assert(strcmp(op_sampler->operation_name, operation_name) == 0);
        jaeger_guaranteed_throughput_probabilistic_sampler* g =
            &op_sampler->sampler;
        const bool decision =
            ((jaeger_sampler*) g)
                ->is_sampled(
                    (jaeger_sampler*) g, trace_id, operation_name, tags);
        jaeger_mutex_unlock(&s->mutex);
        return decision;
    }

    if (jaeger_vector_length(&s->op_samplers) >= s->max_operations) {
        goto use_default_sampler;
    }

    char* operation_name_copy = jaeger_strdup(operation_name);
    if (operation_name_copy == NULL) {
        goto use_default_sampler;
    }
    const jaeger_operation_sampler key = {.operation_name =
                                              operation_name_copy};
    const int pos =
        jaeger_vector_lower_bound(&s->op_samplers, &key, &op_name_cmp);
    jaeger_operation_sampler* op_sampler_ptr =
        jaeger_vector_insert(&s->op_samplers, pos);
    if (op_sampler_ptr == NULL) {
        goto use_default_sampler;
    }
    op_sampler_ptr->operation_name = operation_name_copy;
    jaeger_guaranteed_throughput_probabilistic_sampler_init(
        &op_sampler_ptr->sampler,
        s->lower_bound,
        s->default_sampler.sampling_rate);

    const bool decision =
        ((jaeger_sampler*) &op_sampler_ptr->sampler)
            ->is_sampled((jaeger_sampler*) &op_sampler_ptr->sampler,
                         trace_id,
                         operation_name,
                         tags);
    jaeger_mutex_unlock(&s->mutex);
    return decision;

use_default_sampler : {
    const bool decision =
        ((jaeger_sampler*) &s->default_sampler)
            ->is_sampled((jaeger_sampler*) &s->default_sampler,
                         trace_id,
                         operation_name,
                         tags);
    jaeger_mutex_unlock(&s->mutex);
    return decision;
}
}

static void jaeger_adaptive_sampler_destroy(jaeger_destructible* sampler)
{
    assert(sampler != NULL);
    jaeger_adaptive_sampler* s = (jaeger_adaptive_sampler*) sampler;
    JAEGERTRACINGC_VECTOR_FOR_EACH(&s->op_samplers,
                                   jaeger_operation_sampler_destroy,
                                   jaeger_operation_sampler);
    jaeger_vector_destroy(&s->op_samplers);
    jaeger_mutex_destroy(&s->mutex);
}

bool jaeger_adaptive_sampler_init(
    jaeger_adaptive_sampler* sampler,
    const jaeger_per_operation_strategy* strategies,
    int max_operations)
{
    assert(sampler != NULL);
    if (!jaeger_vector_init(&sampler->op_samplers,
                            sizeof(jaeger_operation_sampler))) {
        return false;
    }
    if (!samplers_from_strategies(strategies, &sampler->op_samplers)) {
        jaeger_vector_destroy(&sampler->op_samplers);
        return false;
    }
    jaeger_probabilistic_sampler_init(&sampler->default_sampler,
                                      strategies->default_sampling_probability);
    sampler->lower_bound = strategies->default_lower_bound_traces_per_second;
    sampler->max_operations = max_operations;
    sampler->mutex = (jaeger_mutex) JAEGERTRACINGC_MUTEX_INIT;
    ((jaeger_sampler*) sampler)->is_sampled =
        &jaeger_adaptive_sampler_is_sampled;
    ((jaeger_destructible*) sampler)->destroy =
        &jaeger_adaptive_sampler_destroy;
    return true;
}

static inline bool
jaeger_adaptive_sampler_update(jaeger_adaptive_sampler* sampler,
                               const jaeger_per_operation_strategy* strategies)
{
    assert(sampler != NULL);
    assert(strategies != NULL);
    bool success = true;
    const double lower_bound =
        strategies->default_lower_bound_traces_per_second;
    jaeger_mutex_lock(&sampler->mutex);
    for (int i = 0; i < (int) strategies->n_per_operation_strategy; i++) {
        const jaeger_operation_strategy* strategy =
            strategies->per_operation_strategy[i];
        bool found_match = false;
        if (strategy == NULL) {
            jaeger_log_warn("Encountered null operation strategy");
            continue;
        }
        if (strategy->probabilistic == NULL) {
            jaeger_log_warn("Encountered null probabilistic strategy");
            continue;
        }

        jaeger_operation_sampler* op_sampler =
            jaeger_adaptive_sampler_find_sampler(sampler, strategy->operation);
        if (op_sampler != NULL) {
            jaeger_guaranteed_throughput_probabilistic_sampler_update(
                &op_sampler->sampler,
                lower_bound,
                strategy->probabilistic->sampling_rate);
            found_match = true;
        }

        /* Only consider allocating new samplers if we have not had issues with
         * memory. Otherwise, success will be false and we will skip this part
         * entirely.
         */
        if (!found_match && success) {
            char* operation_name = jaeger_strdup(strategy->operation);
            if (operation_name == NULL) {
                success = false;
                /* Continue with loop so we can update existing samplers despite
                 * memory issues. */
                continue;
            }
            const jaeger_operation_sampler key = {.operation_name =
                                                      operation_name};
            const int pos = jaeger_vector_lower_bound(
                &sampler->op_samplers, &key, &op_name_cmp);
            jaeger_operation_sampler* op_sampler_ptr =
                jaeger_vector_insert(&sampler->op_samplers, pos);
            if (op_sampler_ptr == NULL) {
                success = false;
                continue;
            }
            jaeger_guaranteed_throughput_probabilistic_sampler_init(
                &op_sampler_ptr->sampler,
                lower_bound,
                strategy->probabilistic->sampling_rate);
        }
    }
    jaeger_mutex_unlock(&sampler->mutex);
    return success;
}

static inline bool jaeger_http_sampling_manager_format_request(
    jaeger_http_sampling_manager* manager,
    const jaeger_url* url,
    const jaeger_host_port* sampling_host_port)
{
    typedef struct str_segment {
        int off;
        int len;
    } str_segment;

    assert(manager != NULL);
    assert(url != NULL);
    assert(sampling_host_port != NULL);

    str_segment path_segment = {.off = -1, .len = 0};
    if (url->parts.field_set & ((uint8_t) UF_PATH)) {
        path_segment = (str_segment){.off = url->parts.field_data[UF_PATH].off,
                                     .len = url->parts.field_data[UF_PATH].len};
    }
    const int path_len = path_segment.len;
    char path_buffer[path_len + 1];
    path_buffer[path_len] = '\0';
    if (path_len > 0) {
        memcpy(&path_buffer[0], &url->str[path_segment.off], path_segment.len);
    }

    char host_port_buffer[256];
    int result = jaeger_host_port_format(
        sampling_host_port, &host_port_buffer[0], sizeof(host_port_buffer));
    if (result > (int) sizeof(host_port_buffer)) {
        jaeger_log_error(
            "Cannot write entire sampling server host port to buffer, "
            "buffer size = %zu, host port string length = %d",
            sizeof(manager->request_buffer),
            manager->request_length);
        return false;
    }

    result = snprintf(&manager->request_buffer[0],
                      sizeof(manager->request_buffer),
                      "GET /%s?service=%s\r\n"
                      "Host: %s\r\n"
                      "User-Agent: jaegertracing/%s\r\n\r\n",
                      &path_buffer[0],
                      manager->service_name,
                      &host_port_buffer[0],
                      JAEGERTRACINGC_CLIENT_VERSION);
    if (result > (int) sizeof(manager->request_buffer)) {
        jaeger_log_error("Cannot write entire HTTP sampling request to "
                         "buffer, buffer size = %zu, request length = %d",
                         sizeof(manager->request_buffer),
                         manager->request_length);
        return false;
    }
    manager->request_length = result;
    return true;
}

enum { http_parsing_state_write, http_parsing_state_read };

typedef struct parsing_context {
    jaeger_http_sampling_manager* manager;
    int state;
} parsing_context;

static int
jaeger_http_sampling_manager_parser_on_headers_complete(http_parser* parser)
{
    assert(parser != NULL);
    assert(parser->data != NULL);
    if (parser->status_code != HTTP_OK) {
        jaeger_log_error("HTTP sampling manager cannot retrieve sampling "
                         "strategies, HTTP status code = %d",
                         parser->status_code);
        return 1;
    }
    return 0;
}

static int jaeger_http_sampling_manager_parser_on_body(http_parser* parser,
                                                       const char* at,
                                                       size_t len)
{
    assert(parser != NULL);
    assert(parser->data != NULL);
    parsing_context* ctx = parser->data;
    jaeger_http_sampling_manager* manager = ctx->manager;
    assert(manager != NULL);

    char* ptr = jaeger_vector_extend(
        &manager->response, jaeger_vector_length(&manager->response), len);
    memcpy(ptr, at, len);
    return 0;
}

static int
jaeger_http_sampling_manager_parser_on_message_complete(http_parser* parser)
{
    assert(parser != NULL);
    assert(parser->data != NULL);
    parsing_context* ctx = parser->data;
    ctx->state = http_parsing_state_write;
    return 0;
}

static inline bool
jaeger_http_sampling_manager_connect(jaeger_http_sampling_manager* manager,
                                     const jaeger_host_port* sampling_host_port)
{
    assert(manager != NULL);
    struct addrinfo* host_addrs = NULL;
    if (!jaeger_host_port_resolve(
            sampling_host_port, SOCK_STREAM, &host_addrs)) {
        return false;
    }

    bool success = false;
    for (struct addrinfo* addr_iter = host_addrs; addr_iter != NULL;
         addr_iter = addr_iter->ai_next) {
        const int fd = socket(AF_INET, SOCK_STREAM, 0);
        if (fd < 0) {
            continue;
        }

        if (connect(fd, addr_iter->ai_addr, addr_iter->ai_addrlen) == 0) {
            manager->fd = fd;
            success = true;
            break;
        }

        close(fd);
    }
    if (!success) {
        jaeger_log_error("Cannot connect to sampling server URL, URL = \"%s\"",
                         manager->sampling_server_url.str);
    }
    freeaddrinfo(host_addrs);
    return success;
}

static inline void
jaeger_http_sampling_manager_destroy(jaeger_http_sampling_manager* manager)
{
    if (manager != NULL) {
        if (manager->fd >= 0) {
            close(manager->fd);
        }
        jaeger_url_destroy(&manager->sampling_server_url);
        if (manager->service_name != NULL) {
            jaeger_free(manager->service_name);
            manager->service_name = NULL;
        }
        if (manager->parser.data != NULL) {
            jaeger_free(manager->parser.data);
            manager->parser.data = NULL;
        }
        jaeger_vector_destroy(&manager->response);
    }
}

static inline bool
jaeger_http_sampling_manager_init(jaeger_http_sampling_manager* manager,
                                  const char* sampling_server_url,
                                  const char* service_name)
{
    assert(manager != NULL);
    assert(service_name != NULL && strlen(service_name) > 0);

    manager->service_name = jaeger_strdup(service_name);
    if (manager->service_name == NULL) {
        return false;
    }

    sampling_server_url =
        (sampling_server_url != NULL && strlen(sampling_server_url) > 0)
            ? sampling_server_url
            : "http://localhost:5778/sampling";
    jaeger_host_port sampling_host_port = {.host = NULL, .port = 0};
    if (!jaeger_url_init(&manager->sampling_server_url, sampling_server_url) ||
        !jaeger_host_port_from_url(&sampling_host_port,
                                   &manager->sampling_server_url)) {
        goto cleanup_manager;
    }
    if (!jaeger_http_sampling_manager_connect(manager, &sampling_host_port)) {
        goto cleanup_host_port;
    }

    http_parser_init(&manager->parser, HTTP_RESPONSE);
    parsing_context* ctx = jaeger_malloc(sizeof(parsing_context));
    if (ctx == NULL) {
        jaeger_log_error("Cannot allocate parsing context");
        goto cleanup_host_port;
    }
    memset(ctx, 0, sizeof(*ctx));
    manager->parser.data = ctx;
    manager->settings.on_headers_complete =
        &jaeger_http_sampling_manager_parser_on_headers_complete;
    manager->settings.on_body = &jaeger_http_sampling_manager_parser_on_body;
    manager->settings.on_message_complete =
        &jaeger_http_sampling_manager_parser_on_message_complete;

    if (!jaeger_vector_init(&manager->response, sizeof(char))) {
        goto cleanup_ctx;
    }
    if (!jaeger_vector_reserve(
            &manager->response,
            JAEGERTRACINGC_HTTP_SAMPLING_MANAGER_REQUEST_MAX_LEN)) {
        goto cleanup_vec;
    }

    const bool decision = jaeger_http_sampling_manager_format_request(
        manager, &manager->sampling_server_url, &sampling_host_port);
    jaeger_host_port_destroy(&sampling_host_port);
    return decision;

cleanup_vec:
    jaeger_vector_destroy(&manager->response);
cleanup_ctx:
    jaeger_free(ctx);
cleanup_host_port:
    jaeger_host_port_destroy(&sampling_host_port);
cleanup_manager:
    jaeger_http_sampling_manager_destroy(manager);
    return false;
}

#define ERR_FMT                                                   \
    "message = \"%s\", source = \"%s\", line = %d, column = %d, " \
    "position = %d"

#define ERR_ARGS(source) err.text, (source), err.line, err.column, err.position

#define PRINT_ERR_MSG(source)                        \
    do {                                             \
        jaeger_log_error(ERR_FMT, ERR_ARGS(source)); \
    } while (0)

static inline bool parse_probabilistic_sampling_json(
    jaeger_probabilistic_strategy* strategy, json_t* json, const char* source)
{
    assert(strategy != NULL);
    assert(json != NULL);
    json_error_t err;
    const int result = json_unpack_ex(
        json, &err, 0, "{sf}", "samplingRate", &strategy->sampling_rate);
    if (result != 0) {
        PRINT_ERR_MSG(source);
        return false;
    }
    return true;
}

static inline bool parse_rate_limiting_sampling_json(
    jaeger_rate_limiting_strategy* strategy, json_t* json, const char* source)
{
    assert(strategy != NULL);
    assert(json != NULL);
    json_error_t err;
    const int result = json_unpack_ex(json,
                                      &err,
                                      0,
                                      "{si}",
                                      "maxTracesPerSecond",
                                      &strategy->max_traces_per_second);
    if (result != 0) {
        PRINT_ERR_MSG(source);
        return false;
    }
    return true;
}

static inline bool parse_per_operation_sampling_json(
    jaeger_per_operation_strategy* strategy, json_t* json, const char* source)
{
    assert(strategy != NULL);
    assert(json != NULL);

    json_t* array = NULL;
    json_error_t err;
    const int result =
        json_unpack_ex(json,
                       &err,
                       0,
                       "{sf sf s?o}",
                       "defaultSamplingProbability",
                       &strategy->default_sampling_probability,
                       "defaultLowerBoundTracesPerSecond",
                       &strategy->default_lower_bound_traces_per_second,
                       "perOperationStrategies",
                       &array);

    if (result != 0) {
        PRINT_ERR_MSG(source);
        return false;
    }

    if (array == NULL) {
        strategy->n_per_operation_strategy = 0;
        strategy->per_operation_strategy = NULL;
        return true;
    }

    if (!json_is_array(array)) {
        char* json_str = json_dumps(array, 0);
        if (json_str != NULL) {
            jaeger_log_error("perOperationStrategies must be an array, %s",
                             json_str);
            jaeger_free(json_str);
        }
        else {
            jaeger_log_error("perOperationStrategies must be an array");
        }
        return false;
    }

    strategy->n_per_operation_strategy = json_array_size(array);
    strategy->per_operation_strategy =
        jaeger_malloc(sizeof(jaeger_operation_strategy*) *
                      strategy->n_per_operation_strategy);
    if (strategy->per_operation_strategy == NULL) {
        jaeger_log_error("Cannot allocate perOperationStrategies for response "
                         "JSON, num operation strategies = %zu",
                         strategy->n_per_operation_strategy);
        return false;
    }

    bool success = true;
    int num_allocated = 0;
    for (int i = 0; i < (int) strategy->n_per_operation_strategy; i++) {
        jaeger_operation_strategy* op_strategy =
            jaeger_malloc(sizeof(jaeger_operation_strategy));
        if (op_strategy == NULL) {
            jaeger_log_error(
                "Cannot allocate operation strategy, num operation "
                "strategies = %zu",
                strategy->n_per_operation_strategy);
            success = false;
            break;
        }
        num_allocated++;
        strategy->per_operation_strategy[i] = op_strategy;

        op_strategy->probabilistic =
            jaeger_malloc(sizeof(jaeger_probabilistic_strategy));
        if (op_strategy->probabilistic == NULL) {
            jaeger_log_error("Cannot allocate probabilistic strategy, num "
                             "operation strategies = %zu",
                             strategy->n_per_operation_strategy);
            success = false;
            break;
        }
        *op_strategy->probabilistic = (jaeger_probabilistic_strategy)
            JAEGERTRACINGC_PROBABILISTIC_STRATEGY_INIT;

        /* Borrow reference */
        json_t* op_json = json_array_get(array, i);
        assert(op_json != NULL);
        json_t* probabilistic_json = NULL;
        const char* operation_name = NULL;
        const int result = json_unpack_ex(op_json,
                                          &err,
                                          0,
                                          "{ss so}",
                                          "operation",
                                          &operation_name,
                                          "probabilisticSampling",
                                          &probabilistic_json);
        if (result != 0) {
            PRINT_ERR_MSG(source);
            success = false;
            break;
        }

        op_strategy->operation = jaeger_strdup(operation_name);
        if (op_strategy->operation == NULL) {
            jaeger_log_error("Cannot allocate operation name for operation "
                             "strategy, operation name = \"%s\"",
                             operation_name);
            success = false;
            break;
        }
        if (!parse_probabilistic_sampling_json(
                op_strategy->probabilistic, probabilistic_json, source)) {
            success = false;
            break;
        }
    }

    if (!success && strategy->per_operation_strategy != NULL) {
        strategy->n_per_operation_strategy = num_allocated;
        jaeger_per_operation_strategy_destroy(strategy);
    }

    return success;
}

static inline bool jaeger_http_sampling_manager_parse_response_json(
    jaeger_http_sampling_manager* manager, jaeger_strategy_response* response)
{
    assert(manager != NULL);
    assert(response != NULL);

    json_error_t err;
    json_t* response_json = json_loads(
        jaeger_vector_get(&manager->response, 0), JSON_REJECT_DUPLICATES, &err);

    if (response_json == NULL) {
        jaeger_log_error(
            "JSON decoding error, " ERR_FMT,
            ERR_ARGS((char*) jaeger_vector_get(&manager->response, 0)));
        return false;
    }

    /* Although we could shrink the response buffer at this point, the next
     * response will probably need a response buffer about the same size. To
     * avoid incurring performance hits for multiple allocations, we can
     * keep the buffer until the manager is freed.
     */

    json_t* probabilistic_json = NULL;
    json_t* rate_limiting_json = NULL;
    json_t* per_operation_json = NULL;
    const int result = json_unpack_ex(response_json,
                                      &err,
                                      0,
                                      "{s?o s?o s?o}",
                                      "probabilisticSampling",
                                      &probabilistic_json,
                                      "rateLimitingSampling",
                                      &rate_limiting_json,
                                      "operationSampling",
                                      &per_operation_json);
    bool success = true;
    if (result != 0) {
        PRINT_ERR_MSG((char*) jaeger_vector_get(&manager->response, 0));
        success = false;
        goto cleanup;
    }

    if (probabilistic_json != NULL) {
        response->strategy_case =
            JAEGERTRACINGC_STRATEGY_RESPONSE_TYPE(PROBABILISTIC);
        response->probabilistic =
            jaeger_malloc(sizeof(jaeger_probabilistic_strategy));
        if (response->probabilistic == NULL) {
            jaeger_log_error("Cannot allocate probabilistic strategy for HTTP "
                             "sampling response");
            success = false;
        }
        else {
            *response->probabilistic = (jaeger_probabilistic_strategy)
                JAEGERTRACINGC_PROBABILISTIC_STRATEGY_INIT;
            success = parse_probabilistic_sampling_json(
                response->probabilistic,
                probabilistic_json,
                jaeger_vector_get(&manager->response, 0));
        }
    }
    else if (rate_limiting_json != NULL) {
        response->strategy_case =
            JAEGERTRACINGC_STRATEGY_RESPONSE_TYPE(RATE_LIMITING);
        response->rate_limiting =
            jaeger_malloc(sizeof(jaeger_rate_limiting_strategy));
        if (response->rate_limiting == NULL) {
            jaeger_log_error("Cannot allocate rate limiting strategy for HTTP "
                             "sampling response");
            success = false;
        }
        else {
            success = parse_rate_limiting_sampling_json(
                response->rate_limiting,
                rate_limiting_json,
                jaeger_vector_get(&manager->response, 0));
        }
    }
    else {
        if (per_operation_json == NULL) {
            jaeger_log_warn(
                "JSON response contains no strategies, response = \"%s\"",
                (char*) jaeger_vector_get(&manager->response, 0));
            success = false;
            goto cleanup;
        }

        response->strategy_case =
            JAEGERTRACINGC_STRATEGY_RESPONSE_TYPE(PER_OPERATION);
        response->per_operation =
            jaeger_malloc(sizeof(jaeger_per_operation_strategy));
        if (response->per_operation == NULL) {
            jaeger_log_error("Cannot allocate per operation strategy for HTTP "
                             "sampling response");
            success = false;
        }
        else {
            success = parse_per_operation_sampling_json(
                response->per_operation,
                per_operation_json,
                jaeger_vector_get(&manager->response, 0));
        }
    }

cleanup:
    json_decref(response_json);
    return success;
}

#undef PRINT_ERR_MSG
#undef ERR_ARGS
#undef ERR_FMT

static inline bool jaeger_http_sampling_manager_get_sampling_strategies(
    jaeger_http_sampling_manager* manager, jaeger_strategy_response* response)
{
    assert(manager != NULL);
    assert(response != NULL);
    parsing_context* ctx = manager->parser.data;
    assert(ctx != NULL);
    *ctx = (parsing_context){.manager = manager,
                             .state = http_parsing_state_write};
    jaeger_vector_clear(&manager->response);
    const int num_written = write(
        manager->fd, &manager->request_buffer[0], manager->request_length);
    if (num_written != manager->request_length) {
        jaeger_log_error("Cannot write entire HTTP sampling request, "
                         "num written = %d, request length = %d, errno = %d",
                         num_written,
                         manager->request_length,
                         errno);
        return false;
    }

    ctx->state = http_parsing_state_read;
    char chunk_buffer[JAEGERTRACINGC_HTTP_SAMPLING_MANAGER_REQUEST_MAX_LEN];
    int num_read = read(manager->fd, &chunk_buffer[0], sizeof(chunk_buffer));
    while (num_read > 0) {
        const int num_parsed = http_parser_execute(
            &manager->parser, &manager->settings, &chunk_buffer[0], num_read);
        if (num_parsed != num_read) {
            return false;
        }
        parsing_context* ctx = manager->parser.data;
        assert(ctx != NULL);
        if (ctx->state != http_parsing_state_read) {
            break;
        }
        num_read = read(manager->fd, &chunk_buffer[0], sizeof(chunk_buffer));
    }

    char* null_byte_ptr = jaeger_vector_append(&manager->response);
    *null_byte_ptr = '\0';

    return jaeger_http_sampling_manager_parse_response_json(manager, response);
}

static bool
jaeger_remotely_controlled_sampler_is_sampled(jaeger_sampler* sampler,
                                              const jaeger_trace_id* trace_id,
                                              const char* operation_name,
                                              jaeger_vector* tags)
{
    assert(sampler != NULL);
    jaeger_remotely_controlled_sampler* s =
        (jaeger_remotely_controlled_sampler*) sampler;
    jaeger_mutex_lock(&s->mutex);
    jaeger_sampler* inner_sampler =
        jaeger_sampler_choice_get_sampler(&s->sampler);
    assert(inner_sampler != NULL);
    const bool result = inner_sampler->is_sampled(
        inner_sampler, trace_id, operation_name, tags);
    jaeger_mutex_unlock(&s->mutex);
    return result;
}

static void
jaeger_remotely_controlled_sampler_destroy(jaeger_destructible* sampler)
{
    jaeger_remotely_controlled_sampler* s =
        (jaeger_remotely_controlled_sampler*) sampler;
    jaeger_sampler_choice* sampler_choice = &s->sampler;
    jaeger_sampler_choice_destroy(sampler_choice);
    jaeger_http_sampling_manager_destroy(&s->manager);
    jaeger_mutex_destroy(&s->mutex);
}

static inline bool jaeger_remotely_controlled_sampler_update_adaptive_sampler(
    jaeger_remotely_controlled_sampler* sampler,
    const jaeger_per_operation_strategy* strategies)
{
    assert(sampler != NULL);
    assert(strategies != NULL);
    if (sampler->sampler.type == jaeger_adaptive_sampler_type) {
        return jaeger_adaptive_sampler_update(
            &sampler->sampler.adaptive_sampler, strategies);
    }
    jaeger_sampler_choice_destroy(&sampler->sampler);
    sampler->sampler.type = jaeger_adaptive_sampler_type;
    return jaeger_adaptive_sampler_init(&sampler->sampler.adaptive_sampler,
                                        strategies,
                                        sampler->max_operations);
}

bool jaeger_remotely_controlled_sampler_update(
    jaeger_remotely_controlled_sampler* sampler)
{
    assert(sampler != NULL);
    jaeger_strategy_response response = JAEGERTRACINGC_STRATEGY_RESPONSE_INIT;
    const bool result = jaeger_http_sampling_manager_get_sampling_strategies(
        &sampler->manager, &response);
    if (!result) {
        jaeger_log_error("Cannot get sampling strategies, will retry later");
        if (sampler->metrics != NULL) {
            jaeger_counter* query_failure =
                sampler->metrics->sampler_query_failure;
            assert(query_failure != NULL);
            query_failure->inc(query_failure, 1);
        }
        return false;
    }

    bool success = true;
    jaeger_mutex_lock(&sampler->mutex);

    if (sampler->metrics != NULL) {
        jaeger_counter* retrieved = sampler->metrics->sampler_retrieved;
        assert(retrieved != NULL);
        retrieved->inc(retrieved, 1);
    }

    switch (response.strategy_case) {
    case JAEGERTRACINGC_STRATEGY_RESPONSE_TYPE(PER_OPERATION): {
        if (!jaeger_remotely_controlled_sampler_update_adaptive_sampler(
                sampler, response.per_operation)) {
            jaeger_log_error("Cannot update adaptive sampler in remotely "
                             "controlled "
                             "sampler");
            success = false;
        }
    } break;
    case JAEGERTRACINGC_STRATEGY_RESPONSE_TYPE(PROBABILISTIC): {
        success = (response.probabilistic != NULL);
        if (success) {
            jaeger_sampler_choice_destroy(&sampler->sampler);
            sampler->sampler.type = jaeger_probabilistic_sampler_type;
            jaeger_probabilistic_sampler_init(
                &sampler->sampler.probabilistic_sampler,
                JAEGERTRACINGC_CLAMP(
                    response.probabilistic->sampling_rate, 0, 1));
        }
        else {
            jaeger_log_error("Received null probabilistic strategy");
        }
    } break;
    default: {
        success = (response.strategy_case ==
                       JAEGERTRACINGC_STRATEGY_RESPONSE_TYPE(RATE_LIMITING) &&
                   response.rate_limiting != NULL);
        if (response.strategy_case !=
            JAEGERTRACINGC_STRATEGY_RESPONSE_TYPE(RATE_LIMITING)) {
            jaeger_log_error("Invalid strategy type in response, type = %d",
                             response.strategy_case);
        }
        else if (response.rate_limiting == NULL) {
            jaeger_log_error("Received null rate limiting strategy");
        }
        else {
            jaeger_sampler_choice_destroy(&sampler->sampler);
            int max_traces_per_second =
                response.rate_limiting->max_traces_per_second;
            sampler->sampler.type = jaeger_rate_limiting_sampler_type;
            jaeger_rate_limiting_sampler_init(
                &sampler->sampler.rate_limiting_sampler, max_traces_per_second);
        }
    } break;
    }

    if (sampler->metrics != NULL) {
        jaeger_counter* sampler_update_metric =
            (success ? sampler->metrics->sampler_updated
                     : sampler->metrics->sampler_update_failure);
        assert(sampler_update_metric != NULL);
        sampler_update_metric->inc(sampler_update_metric, 1);
    }

    jaeger_mutex_unlock(&sampler->mutex);
    jaeger_strategy_response_destroy(&response);
    return success;
}

bool jaeger_remotely_controlled_sampler_init(
    jaeger_remotely_controlled_sampler* sampler,
    const char* service_name,
    const char* sampling_server_url,
    const jaeger_sampler_choice* initial_sampler,
    int max_operations,
    jaeger_metrics* metrics)
{
    assert(sampler != NULL);
    assert(service_name != NULL && strlen(service_name) > 0);
    max_operations =
        (max_operations <= 0) ? DEFAULT_MAX_OPERATIONS : max_operations;
    *sampler = (jaeger_remotely_controlled_sampler){
        {{&jaeger_remotely_controlled_sampler_destroy},
         &jaeger_remotely_controlled_sampler_is_sampled},
        JAEGERTRACINGC_SAMPLER_CHOICE_INIT,
        max_operations,
        metrics,
        JAEGERTRACINGC_HTTP_SAMPLING_MANAGER_INIT,
        JAEGERTRACINGC_MUTEX_INIT};

    if (initial_sampler != NULL) {
        sampler->sampler = *initial_sampler;
    }
    else {
        sampler->sampler.type = jaeger_probabilistic_sampler_type;
        jaeger_probabilistic_sampler_init(
            &sampler->sampler.probabilistic_sampler, DEFAULT_SAMPLING_RATE);
    }

    if (!jaeger_http_sampling_manager_init(
            &sampler->manager, sampling_server_url, service_name)) {
        jaeger_log_error("Cannot initialize HTTP manager for remotely "
                         "controlled sampler");
        return false;
    }

    return true;
}
