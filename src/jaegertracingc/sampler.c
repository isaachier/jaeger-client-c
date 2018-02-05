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
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#define SAMPLER_GROWTH_FACTOR 2

static bool jaeger_const_sampler_is_sampled(jaeger_sampler* sampler,
                                            const jaeger_trace_id* trace_id,
                                            const char* operation_name,
                                            jaeger_tag_list* tags)
{
    (void) trace_id;
    (void) operation_name;
    jaeger_const_sampler* s = (jaeger_const_sampler*) sampler;
    if (tags != NULL) {
        jaeger_tag tag = JAEGERTRACING__PROTOBUF__TAG__INIT;
        tag.key = JAEGERTRACINGC_SAMPLER_TYPE_TAG_KEY;
        tag.value_case = JAEGERTRACING__PROTOBUF__TAG__VALUE_STR_VALUE;
        tag.str_value = JAEGERTRACINGC_SAMPLER_TYPE_CONST;
        jaeger_tag_list_append(tags, &tag);

        tag.key = JAEGERTRACINGC_SAMPLER_PARAM_TAG_KEY;
        tag.value_case = JAEGERTRACING__PROTOBUF__TAG__VALUE_BOOL_VALUE;
        tag.bool_value = s->decision;
        jaeger_tag_list_append(tags, &tag);
    }
    return s->decision;
}

static void jaeger_sampler_noop_close(jaeger_sampler* sampler)
{
    (void) sampler;
}

void jaeger_const_sampler_init(jaeger_const_sampler* sampler, bool decision)
{
    assert(sampler != NULL);
    sampler->is_sampled = &jaeger_const_sampler_is_sampled;
    sampler->close = &jaeger_sampler_noop_close;
}

static bool
jaeger_probabilistic_sampler_is_sampled(jaeger_sampler* sampler,
                                        const jaeger_trace_id* trace_id,
                                        const char* operation_name,
                                        jaeger_tag_list* tags)
{
    (void) trace_id;
    (void) operation_name;
    jaeger_probabilistic_sampler* s = (jaeger_probabilistic_sampler*) sampler;
#ifdef HAVE_RAND_R
    const double threshold = ((double) rand_r(&s->seed)) / RAND_MAX;
#else
    const double threshold = ((double) rand()) / RAND_MAX;
#endif /* HAVE_RAND_R */
    const bool decision = (s->sampling_rate >= threshold);
    if (tags != NULL) {
        jaeger_tag tag = JAEGERTRACING__PROTOBUF__TAG__INIT;
        tag.key = JAEGERTRACINGC_SAMPLER_TYPE_TAG_KEY;
        tag.value_case = JAEGERTRACING__PROTOBUF__TAG__VALUE_STR_VALUE;
        tag.str_value = JAEGERTRACINGC_SAMPLER_TYPE_PROBABILISTIC;
        jaeger_tag_list_append(tags, &tag);

        tag.key = JAEGERTRACINGC_SAMPLER_PARAM_TAG_KEY;
        tag.value_case = JAEGERTRACING__PROTOBUF__TAG__VALUE_DOUBLE_VALUE;
        tag.double_value = s->sampling_rate;
        jaeger_tag_list_append(tags, &tag);
    }
    return decision;
}

void jaeger_probabilistic_sampler_init(jaeger_probabilistic_sampler* sampler,
                                       double sampling_rate)
{
    assert(sampler != NULL);
    sampler->is_sampled = &jaeger_probabilistic_sampler_is_sampled;
    sampler->close = &jaeger_sampler_noop_close;
    sampler->sampling_rate =
        (sampling_rate < 0) ? 0 : ((sampling_rate > 1) ? 1 : sampling_rate);
}

static bool
jaeger_rate_limiting_sampler_is_sampled(jaeger_sampler* sampler,
                                        const jaeger_trace_id* trace_id,
                                        const char* operation_name,
                                        jaeger_tag_list* tags)
{
    (void) trace_id;
    (void) operation_name;
    assert(sampler != NULL);
    jaeger_rate_limiting_sampler* s = (jaeger_rate_limiting_sampler*) sampler;
    const bool decision = jaeger_token_bucket_check_credit(&s->tok, 1);
    if (tags != NULL) {
        jaeger_tag tag = JAEGERTRACING__PROTOBUF__TAG__INIT;
        tag.key = JAEGERTRACINGC_SAMPLER_TYPE_TAG_KEY;
        tag.value_case = JAEGERTRACING__PROTOBUF__TAG__VALUE_STR_VALUE;
        tag.str_value = JAEGERTRACINGC_SAMPLER_TYPE_RATE_LIMITING;
        jaeger_tag_list_append(tags, &tag);

        tag.key = JAEGERTRACINGC_SAMPLER_PARAM_TAG_KEY;
        tag.value_case = JAEGERTRACING__PROTOBUF__TAG__VALUE_DOUBLE_VALUE;
        tag.double_value = s->max_traces_per_second;
        jaeger_tag_list_append(tags, &tag);
    }
    return decision;
}

void jaeger_rate_limiting_sampler_init(jaeger_rate_limiting_sampler* sampler,
                                       double max_traces_per_second)
{
    assert(sampler != NULL);
    sampler->is_sampled = &jaeger_rate_limiting_sampler_is_sampled;
    sampler->close = &jaeger_sampler_noop_close;
    jaeger_token_bucket_init(
        &sampler->tok,
        max_traces_per_second,
        (max_traces_per_second < 1) ? 1 : max_traces_per_second);
    sampler->max_traces_per_second = max_traces_per_second;
}

static bool jaeger_guaranteed_throughput_probabilistic_sampler_is_sampled(
    jaeger_sampler* sampler,
    const jaeger_trace_id* trace_id,
    const char* operation_name,
    jaeger_tag_list* tags)
{
    (void) trace_id;
    (void) operation_name;
    assert(sampler != NULL);
    jaeger_guaranteed_throughput_probabilistic_sampler* s =
        (jaeger_guaranteed_throughput_probabilistic_sampler*) sampler;
    bool decision = s->probabilistic_sampler.is_sampled(
        (jaeger_sampler*) &s->probabilistic_sampler,
        trace_id,
        operation_name,
        NULL);
    if (decision) {
        s->lower_bound_sampler.is_sampled(
            (jaeger_sampler*) &s->lower_bound_sampler,
            trace_id,
            operation_name,
            NULL);
        if (tags != NULL) {
            jaeger_tag tag = JAEGERTRACING__PROTOBUF__TAG__INIT;
            tag.key = JAEGERTRACINGC_SAMPLER_TYPE_TAG_KEY;
            tag.value_case = JAEGERTRACING__PROTOBUF__TAG__VALUE_STR_VALUE;
            tag.str_value = JAEGERTRACINGC_SAMPLER_TYPE_PROBABILISTIC;
            jaeger_tag_list_append(tags, &tag);

            tag.key = JAEGERTRACINGC_SAMPLER_PARAM_TAG_KEY;
            tag.value_case = JAEGERTRACING__PROTOBUF__TAG__VALUE_DOUBLE_VALUE;
            tag.double_value = s->probabilistic_sampler.sampling_rate;
            jaeger_tag_list_append(tags, &tag);
        }
        return true;
    }
    decision = s->lower_bound_sampler.is_sampled(
        (jaeger_sampler*) &s->lower_bound_sampler,
        trace_id,
        operation_name,
        NULL);
    if (tags != NULL) {
        jaeger_tag tag = JAEGERTRACING__PROTOBUF__TAG__INIT;
        tag.key = JAEGERTRACINGC_SAMPLER_TYPE_TAG_KEY;
        tag.value_case = JAEGERTRACING__PROTOBUF__TAG__VALUE_STR_VALUE;
        tag.str_value = JAEGERTRACINGC_SAMPLER_TYPE_RATE_LIMITING;
        jaeger_tag_list_append(tags, &tag);

        tag.key = JAEGERTRACINGC_SAMPLER_PARAM_TAG_KEY;
        tag.value_case = JAEGERTRACING__PROTOBUF__TAG__VALUE_DOUBLE_VALUE;
        tag.double_value = s->lower_bound_sampler.max_traces_per_second;
        jaeger_tag_list_append(tags, &tag);
    }
    return decision;
}

static void jaeger_guaranteed_throughput_probabilistic_sampler_close(
    jaeger_sampler* sampler)
{
    assert(sampler != NULL);
    jaeger_guaranteed_throughput_probabilistic_sampler* s =
        (jaeger_guaranteed_throughput_probabilistic_sampler*) sampler;
    s->probabilistic_sampler.close((jaeger_sampler*) &s->probabilistic_sampler);
    s->lower_bound_sampler.close((jaeger_sampler*) &s->lower_bound_sampler);
}

void jaeger_guaranteed_throughput_probabilistic_sampler_init(
    jaeger_guaranteed_throughput_probabilistic_sampler* sampler,
    double lower_bound,
    double sampling_rate)
{
    assert(sampler != NULL);
    sampler->is_sampled =
        &jaeger_guaranteed_throughput_probabilistic_sampler_is_sampled;
    sampler->close = &jaeger_guaranteed_throughput_probabilistic_sampler_close;
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
        sampler->probabilistic_sampler.close(
            (jaeger_sampler*) &sampler->probabilistic_sampler);
        jaeger_probabilistic_sampler_init(&sampler->probabilistic_sampler,
                                          sampling_rate);
    }
    if (sampler->lower_bound_sampler.max_traces_per_second != lower_bound) {
        sampler->lower_bound_sampler.close(
            (jaeger_sampler*) &sampler->lower_bound_sampler);
        jaeger_rate_limiting_sampler_init(&sampler->lower_bound_sampler,
                                          lower_bound);
    }
}

typedef Jaegertracing__Protobuf__SamplingManager__PerOperationSamplingStrategy__OperationSamplingStrategy
    jaeger_operation_strategy;

static inline jaeger_operation_sampler*
samplers_from_strategies(const jaeger_per_operation_strategy* strategies,
                         int* const num_op_samplers)
{
    assert(strategies != NULL);
    assert(num_op_samplers != NULL);
    *num_op_samplers = 0;
    for (int i = 0; i < strategies->n_per_operation_strategy; i++) {
        const jaeger_operation_strategy* strategy =
            strategies->per_operation_strategy[i];
        if (strategy == NULL) {
            fprintf(stderr, "WARNING: Encountered null operation strategy\n");
            continue;
        }
        if (strategy->probabilistic == NULL) {
            fprintf(stderr,
                    "WARNING: Encountered null probabilistic strategy\n");
            continue;
        }
        (*num_op_samplers)++;
    }

    const size_t op_samplers_size =
        sizeof(jaeger_operation_sampler) * (*num_op_samplers);
    jaeger_operation_sampler* op_samplers = jaeger_malloc(op_samplers_size);
    if (op_samplers == NULL) {
        fprintf(stderr, "ERROR: Cannot allocate per operation samplers\n");
        return NULL;
    }
    memset(op_samplers, 0, op_samplers_size);

    bool success = true;
    int index = 0;
    for (int i = 0; i < strategies->n_per_operation_strategy; i++) {
        const jaeger_operation_strategy* strategy =
            strategies->per_operation_strategy[i];
        if (strategy == NULL || strategy->probabilistic == NULL) {
            continue;
        }
        jaeger_operation_sampler* op_sampler = &op_samplers[index];
        op_sampler->operation_name = jaeger_strdup(strategy->operation);
        if (op_sampler->operation_name == NULL) {
            fprintf(stderr,
                    "ERROR: Cannot allocate operation sampler, "
                    "operation name = \"%s\"\n",
                    strategy->operation);
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
            jaeger_operation_sampler_destroy(&op_samplers[i]);
        }
        jaeger_free(op_samplers);
        return NULL;
    }

    return op_samplers;
}

static inline bool
jaeger_adaptive_sampler_resize_op_samplers(jaeger_adaptive_sampler* sampler)
{
    assert(sampler != NULL);
    assert(sampler->num_op_samplers <= sampler->op_samplers_capacity);
    const int new_capacity =
        JAEGERTRACINGC_MIN(sampler->num_op_samplers * SAMPLER_GROWTH_FACTOR,
                           sampler->max_operations);
    jaeger_operation_sampler* new_op_samplers = jaeger_realloc(
        sampler->op_samplers, sizeof(jaeger_operation_sampler) * new_capacity);
    if (new_op_samplers == NULL) {
        return false;
    }

    sampler->op_samplers = new_op_samplers;
    sampler->op_samplers_capacity = new_capacity;
    return true;
}

static bool jaeger_adaptive_sampler_is_sampled(jaeger_sampler* sampler,
                                               const jaeger_trace_id* trace_id,
                                               const char* operation_name,
                                               jaeger_tag_list* tags)
{
    (void) trace_id;
    (void) operation_name;
    assert(sampler != NULL);
    jaeger_adaptive_sampler* s = (jaeger_adaptive_sampler*) sampler;
    pthread_mutex_lock(&s->mutex);
    for (int i = 0; i < s->num_op_samplers; i++) {
        if (strcmp(s->op_samplers[i].operation_name, operation_name) == 0) {
            jaeger_guaranteed_throughput_probabilistic_sampler* g =
                &s->op_samplers[i].sampler;
            const bool decision = g->is_sampled(
                (jaeger_sampler*) g, trace_id, operation_name, tags);
            pthread_mutex_unlock(&s->mutex);
            return decision;
        }
    }

    if (s->num_op_samplers >= s->max_operations) {
        goto default_sampler;
    }

    if (s->num_op_samplers == s->op_samplers_capacity &&
        !jaeger_adaptive_sampler_resize_op_samplers(s)) {
        fprintf(stderr,
                "ERROR: Cannot allocate more operation samplers "
                "in adaptive sampler\n");
        goto default_sampler;
    }
    jaeger_operation_sampler* op_sampler = &s->op_samplers[s->num_op_samplers];
    op_sampler->operation_name = jaeger_strdup(operation_name);
    if (op_sampler->operation_name == NULL) {
        fprintf(stderr,
                "ERROR: Cannot allocate operation name for new "
                "operation sampler in adaptive sampler\n");
        goto default_sampler;
    }
    jaeger_guaranteed_throughput_probabilistic_sampler_init(
        &op_sampler->sampler, s->lower_bound, s->default_sampler.sampling_rate);
    s->num_op_samplers++;

    const bool decision = op_sampler->sampler.is_sampled(
        (jaeger_sampler*) &op_sampler->sampler, trace_id, operation_name, tags);
    pthread_mutex_unlock(&s->mutex);
    return decision;

default_sampler : {
    const bool decision = s->default_sampler.is_sampled(
        (jaeger_sampler*) &s->default_sampler, trace_id, operation_name, tags);
    pthread_mutex_unlock(&s->mutex);
    return decision;
}
}

static void jaeger_adaptive_sampler_close(jaeger_sampler* sampler)
{
    assert(sampler != NULL);
    jaeger_adaptive_sampler* s = (jaeger_adaptive_sampler*) sampler;
    for (int i = 0; i < s->num_op_samplers; i++) {
        jaeger_operation_sampler_destroy(&s->op_samplers[i]);
    }
    jaeger_free(s->op_samplers);
    pthread_mutex_destroy(&s->mutex);
}

bool jaeger_adaptive_sampler_init(
    jaeger_adaptive_sampler* sampler,
    const jaeger_per_operation_strategy* strategies,
    int max_operations)
{
    assert(sampler != NULL);
    sampler->op_samplers =
        samplers_from_strategies(strategies, &sampler->num_op_samplers);
    if (sampler->op_samplers == NULL) {
        return false;
    }
    sampler->op_samplers_capacity = sampler->num_op_samplers;
    sampler->max_operations = max_operations;
    sampler->mutex = (pthread_mutex_t) PTHREAD_MUTEX_INITIALIZER;
    sampler->is_sampled = &jaeger_adaptive_sampler_is_sampled;
    sampler->close = &jaeger_adaptive_sampler_close;
    return true;
}

void jaeger_adaptive_sampler_update(
    jaeger_adaptive_sampler* sampler,
    const jaeger_per_operation_strategy* strategies)
{
    assert(sampler != NULL);
    assert(strategies != NULL);
    const double lower_bound =
        strategies->default_lower_bound_traces_per_second;
    pthread_mutex_lock(&sampler->mutex);
    for (int i = 0; i < strategies->n_per_operation_strategy; i++) {
        const jaeger_operation_strategy* strategy =
            strategies->per_operation_strategy[i];
        bool found_match = false;
        if (strategy == NULL) {
            fprintf(stderr, "WARNING: Encountered null operation strategy\n");
            continue;
        }
        if (strategy->probabilistic == NULL) {
            fprintf(stderr,
                    "WARNING: Encountered null probabilistic strategy\n");
            continue;
        }

        for (int j = 0; j < sampler->num_op_samplers && !found_match; j++) {
            jaeger_operation_sampler* op_sampler = &sampler->op_samplers[j];
            if (strcmp(op_sampler->operation_name, strategy->operation) == 0) {
                jaeger_guaranteed_throughput_probabilistic_sampler_update(
                    &op_sampler->sampler,
                    lower_bound,
                    strategy->probabilistic->sampling_rate);
                found_match = true;
            }
        }

        if (!found_match) {
            if (sampler->num_op_samplers == sampler->op_samplers_capacity &&
                !jaeger_adaptive_sampler_resize_op_samplers(sampler)) {
                fprintf(stderr,
                        "ERROR: Cannot allocate more operation samplers "
                        "in adaptive sampler\n");
                continue;
            }
            jaeger_operation_sampler* op_sampler =
                &sampler->op_samplers[sampler->num_op_samplers];
            op_sampler->operation_name = jaeger_strdup(strategy->operation);
            if (op_sampler->operation_name == NULL) {
                fprintf(stderr,
                        "ERROR: Cannot allocate operation sampler, "
                        "operation name = \"%s\"\n",
                        strategy->operation);
                continue;
            }
            sampler->num_op_samplers++;
        }
    }
    pthread_mutex_unlock(&sampler->mutex);
}

static inline bool jaeger_http_sampling_manager_format_request(
    jaeger_http_sampling_manager* manager,
    const struct http_parser_url* parsed_url)
{
    assert(manager != NULL);
    assert(parsed_url != NULL);
    int path_len = 2;
    if (parsed_url->field_set & (1 << UF_PATH)) {
        path_len = parsed_url->field_data[(UF_PATH)].len + 1;
    }
    char path[path_len];
    if (path_len > 2) {
        memcpy(
            &path[0],
            &manager->sampling_server_url[parsed_url->field_data[UF_PATH].off],
            path_len);
    }
    else {
        path[0] = '/';
        path[1] = '\0';
    }

    const int host_off = parsed_url->field_set & (1 << UF_HOST)
                             ? parsed_url->field_data[UF_HOST].off
                             : -1;
    const int host_len = parsed_url->field_set & (1 << UF_HOST)
                             ? parsed_url->field_data[UF_HOST].len
                             : 0;
    const int port_off = parsed_url->field_set & (1 << UF_PORT)
                             ? parsed_url->field_data[UF_PORT].off
                             : -1;
    const int port_len = parsed_url->field_set & (1 << UF_PORT)
                             ? parsed_url->field_data[UF_PORT].len
                             : 0;
    const int colon_len = port_len > 0 ? 1 : 0;

    char host_port[host_len + colon_len + port_len + 1];
    host_port[host_len + colon_len + port_len] = '\0';
    int idx = 0;
    if (host_len > 0) {
        memcpy(
            &host_port[idx], &manager->sampling_server_url[host_off], host_len);
        idx += host_len;
    }
    if (colon_len > 0) {
        host_port[idx] = ':';
        idx++;
    }
    if (port_len > 0) {
        memcpy(
            &host_port[idx], &manager->sampling_server_url[port_off], port_len);
        idx += port_len;
    }
    assert(idx == host_len + colon_len + port_len);

    const int result = snprintf(manager->request_buffer,
                                sizeof(manager->request_buffer),
                                "GET %s?service=%s HTTP/1.1\r\n"
                                "Host: %s\r\n"
                                "User-Agent: jaegertracing/%s\r\n\r\n",
                                path,
                                manager->service_name,
                                host_port,
                                JAEGERTRACINGC_CLIENT_VERSION);
    if (result > sizeof(manager->request_buffer)) {
        fprintf(stderr,
                "ERROR: Cannot write entire HTTP sampling request to buffer, "
                "buffer size = %zu, request length = %d\n",
                sizeof(manager->request_buffer),
                result);
        return false;
    }

    manager->request_length = result;
    return true;
}

static int
jaeger_http_sampling_manager_parser_on_headers_complete(http_parser* parser)
{
    assert(parser != NULL);
    if (parser->status_code != HPE_OK) {
        fprintf(stderr,
                "ERROR: HTTP sampling manager failed to retrieve sampling "
                "strategies, HTTP status code = %d\n",
                parser->status_code);
        return 1;
    }
    return 0;
}

#define RESPONSE_BUFFER_GROWTH_FACTOR 2
#define RESPONSE_BUFFER_INIT_SIZE \
    JAEGERTRACINGC_HTTP_SAMPLING_MANAGER_REQUEST_MAX_LEN

static int jaeger_http_sampling_manager_parser_on_body(http_parser* parser,
                                                       const char* at,
                                                       size_t len)
{
    assert(parser != NULL);
    assert(parser->data != NULL);
    jaeger_http_sampling_manager* manager =
        (jaeger_http_sampling_manager*) parser->data;
    if (manager->response_length + len > manager->response_capacity) {
        const int new_response_capacity =
            manager->response_capacity * RESPONSE_BUFFER_GROWTH_FACTOR;
        char* new_response_buffer =
            jaeger_realloc(manager->response_buffer, new_response_capacity);
        if (new_response_buffer == NULL) {
            fprintf(stderr,
                    "ERROR: Cannot allocate larger buffer for JSON sampling "
                    "response, current length = %d, current capacity = %d\n",
                    manager->response_length,
                    manager->response_capacity);
            return 1;
        }
        manager->response_capacity = new_response_capacity;
        manager->response_buffer = new_response_buffer;
    }
    memcpy(&manager->response_buffer[manager->response_length], at, len);
    manager->response_length += len;
    return 0;
}

static inline bool
jaeger_http_sampling_manager_init(jaeger_http_sampling_manager* manager,
                                  const char* sampling_server_url,
                                  const char* service_name)
{
    assert(manager != NULL);
    assert(service_name != NULL);

    manager->service_name = service_name;
    sampling_server_url =
        (sampling_server_url != NULL && strlen(sampling_server_url) > 0)
            ? sampling_server_url
            : "http://localhost:5778/sampling";
    manager->sampling_server_url = sampling_server_url;

    struct http_parser_url parsed_url = {0};
    int result = http_parser_parse_url(manager->sampling_server_url,
                                       strlen(sampling_server_url),
                                       0,
                                       &parsed_url);
    if (result != 0) {
        fprintf(stderr,
                "ERROR: Cannot parse sampling server URL, "
                "sampling server url = \"%s\", "
                "error code = %d\n",
                manager->sampling_server_url,
                result);
        return false;
    }

    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    struct addrinfo* addr = NULL;
    result = getaddrinfo(manager->sampling_server_url, 0, &hints, &addr);
    if (result != 0) {
        fprintf(stderr,
                "ERROR: Cannot resolve sampling server URL for HTTP sampling "
                "manager, sampling server url = \"%s\", error = \"%s\"\n",
                manager->sampling_server_url,
                gai_strerror(result));
        return false;
    }

    bool success = false;
    int socket_fd = -1;
    for (struct addrinfo* addr_iter = addr; addr_iter != NULL;
         addr_iter = addr_iter->ai_next) {
        socket_fd = socket(addr_iter->ai_family,
                           addr_iter->ai_socktype,
                           addr_iter->ai_protocol);
        if (socket_fd < 0) {
            continue;
        }
        if (connect(socket_fd, addr_iter->ai_addr, addr_iter->ai_addrlen)) {
            success = true;
            break;
        }
        close(socket_fd);
    }
    freeaddrinfo(addr);
    if (!success) {
        fprintf(stderr,
                "ERROR: Cannot connect to host for HTTP sampling manager, "
                "sampling server url = \"%s\"\n",
                manager->sampling_server_url);
        return false;
    }

    assert(socket_fd >= 0);
    manager->fd = socket_fd;

    http_parser_init(&manager->parser, HTTP_RESPONSE);
    manager->settings.on_headers_complete =
        &jaeger_http_sampling_manager_parser_on_headers_complete;
    manager->settings.on_body = &jaeger_http_sampling_manager_parser_on_body;

    manager->response_buffer = jaeger_malloc(RESPONSE_BUFFER_INIT_SIZE);
    if (manager->response_buffer == NULL) {
        fprintf(stderr,
                "ERROR: Cannot allocate response buffer in HTTP sampling "
                "manager, size = %d\n",
                RESPONSE_BUFFER_INIT_SIZE);
        close(manager->fd);
        return false;
    }
    manager->response_length = 0;
    manager->response_capacity = RESPONSE_BUFFER_INIT_SIZE;

    return jaeger_http_sampling_manager_format_request(manager, &parsed_url);
}

#define ERR_FMT                                                              \
    "message = \"%s\", source = \"%s\", line = %d, column = %d, position = " \
    "%d\n"

#define ERR_ARGS err.text, err.source, err.line, err.column, err.position

#define PRINT_ERR_MSG                                                         \
    do {                                                                      \
        fprintf(                                                              \
            stderr, "ERROR: Cannot parse JSON response, " ERR_FMT, ERR_ARGS); \
    } while (0)

typedef Jaegertracing__Protobuf__SamplingManager__ProbabilisticSamplingStrategy
    jaeger_probabilistic_strategy;

static inline bool
parse_probabilistic_sampling_json(jaeger_probabilistic_strategy* strategy,
                                  json_t* json)
{
    assert(strategy == NULL);
    assert(json == NULL);
    json_error_t err;
    double sampling_rate = 0;
    const int result =
        json_unpack_ex(json, &err, 0, "{sf}", "samplingRate", &sampling_rate);
    if (result != 0) {
        PRINT_ERR_MSG;
        return false;
    }
    return true;
}

typedef Jaegertracing__Protobuf__SamplingManager__RateLimitingSamplingStrategy
    jaeger_rate_limiting_strategy;

static inline bool
parse_rate_limiting_sampling_json(jaeger_rate_limiting_strategy* strategy,
                                  json_t* json)
{
    assert(strategy == NULL);
    assert(json == NULL);
    json_error_t err;
    double max_traces_per_second = 0;
    const int result = json_unpack_ex(
        json, &err, 0, "{sf}", "maxTracesPerSecond", &max_traces_per_second);
    if (result != 0) {
        PRINT_ERR_MSG;
        return false;
    }
    return true;
}

static inline bool
parse_per_operation_sampling_json(jaeger_per_operation_strategy* strategy,
                                  json_t* json)
{
    assert(strategy == NULL);
    assert(json == NULL);

    json_t* array = NULL;
    json_error_t err;
    const int result =
        json_unpack_ex(json,
                       &err,
                       0,
                       "{sf sf s?O}",
                       "defaultSamplingProbability",
                       &strategy->default_sampling_probability,
                       "defaultLowerBoundTracesPerSecond",
                       &strategy->default_lower_bound_traces_per_second,
                       "perOperationStrategies",
                       &array);

    if (result != 0) {
        PRINT_ERR_MSG;
        return false;
    }

    if (array == NULL) {
        strategy->n_per_operation_strategy = 0;
        strategy->per_operation_strategy = NULL;
        return true;
    }

    if (!json_is_array(array)) {
        fprintf(stderr,
                "ERROR: perOperationStrategies must be an array, JSON: ");
        json_dumpf(array, stderr, 0);
        fprintf(stderr, "\n");
        json_decref(array);
        return false;
    }

    strategy->n_per_operation_strategy = json_array_size(array);
    strategy->per_operation_strategy =
        jaeger_malloc(sizeof(jaeger_operation_strategy*) *
                      strategy->n_per_operation_strategy);
    if (strategy->per_operation_strategy == NULL) {
        fprintf(stderr,
                "ERROR: Cannot allocate perOperationStrategies for response "
                "JSON, num operation strategies = %zu\n",
                strategy->n_per_operation_strategy);
        return false;
    }

    bool success = true;
    int num_allocated = 0;
    for (int i = 0; i < (int) strategy->n_per_operation_strategy; i++) {
        jaeger_operation_strategy* op_strategy =
            jaeger_malloc(sizeof(jaeger_operation_strategy));
        if (op_strategy == NULL) {
            fprintf(stderr,
                    "ERROR: Cannot allocate operation strategy, num operation "
                    "strategies = %zu\n",
                    strategy->n_per_operation_strategy);
            success = false;
            break;
        }
        num_allocated++;
        strategy->per_operation_strategy[i] = op_strategy;

        op_strategy->probabilistic =
            jaeger_malloc(sizeof(jaeger_probabilistic_strategy));
        if (op_strategy->probabilistic == NULL) {
            fprintf(
                stderr,
                "ERROR: Cannot allocate probabilistic strategy, num operation "
                "strategies = %zu\n",
                strategy->n_per_operation_strategy);
            success = false;
            break;
        }

        /* Borrow reference */
        json_t* op_json = json_array_get(array, i);
        assert(op_json != NULL);
        json_t* probabilistic_json = NULL;
        const char* operation_name = NULL;
        const int result = json_unpack_ex(op_json,
                                          &err,
                                          0,
                                          "{ss sO}",
                                          "operation",
                                          &operation_name,
                                          "probabilisticSampling",
                                          &probabilistic_json);
        if (result != 0) {
            PRINT_ERR_MSG;
            success = false;
            break;
        }

        op_strategy->operation = jaeger_strdup(operation_name);
        if (op_strategy->operation == NULL) {
            fprintf(stderr,
                    "ERROR: Cannot allocate operation name for operation "
                    "strategy, operation name = \"%s\"",
                    operation_name);
            success = false;
            break;
        }
        if (!parse_probabilistic_sampling_json(op_strategy->probabilistic,
                                               probabilistic_json)) {
            success = false;
            break;
        }
    }

    if (!success) {
        for (int i = 0; i < num_allocated; i++) {
            jaeger_operation_strategy* op_strategy =
                strategy->per_operation_strategy[i];
            if (op_strategy != NULL) {
                if (op_strategy->probabilistic != NULL) {
                    jaeger_free(op_strategy->probabilistic);
                }
                jaeger_free(op_strategy);
            }
        }
        if (strategy->per_operation_strategy) {
            jaeger_free(strategy->per_operation_strategy);
        }
    }

    json_decref(array);

    return success;
}

typedef Jaegertracing__Protobuf__SamplingManager__SamplingStrategyResponse
    jaeger_strategy_response;

static inline bool jaeger_http_sampling_manager_parse_json_response(
    jaeger_http_sampling_manager* manager, jaeger_strategy_response* response)
{
    assert(manager != NULL);
    assert(response != NULL);

    json_error_t err;
    json_t* response_json =
        json_loads(manager->response_buffer, JSON_REJECT_DUPLICATES, &err);

    if (response_json == NULL) {
        fprintf(stderr, "ERROR: JSON decoding error, " ERR_FMT, ERR_ARGS);
        return false;
    }

    /* Although we could shrink the response buffer at this point, the next
     * response will probably need a response buffer about the same size. To
     * avoid incurring performance hits for multiple allocations, we can keep
     * the buffer until the manager is freed.
     */

    json_t* probabilistic_json = NULL;
    json_t* rate_limiting_json = NULL;
    json_t* per_operation_json = NULL;
    const int result = json_unpack_ex(response_json,
                                      &err,
                                      0,
                                      "{sO* sO* sO*}",
                                      "probabilisticSampling",
                                      &probabilistic_json,
                                      "rateLimitingSampling",
                                      &rate_limiting_json,
                                      "operationSampling",
                                      &per_operation_json);
    bool success = true;
    if (result != 0) {
        PRINT_ERR_MSG;
        success = false;
        goto cleanup;
    }

#define STRATEGY_CASE(x) \
    JAEGERTRACING__PROTOBUF__SAMPLING_MANAGER__SAMPLING_STRATEGY_RESPONSE__STRATEGY_##x

    if (probabilistic_json != NULL) {
        response->strategy_case = STRATEGY_CASE(PROBABILISTIC);
        success = parse_probabilistic_sampling_json(response->probabilistic,
                                                    probabilistic_json);
    }
    else if (rate_limiting_json != NULL) {
        response->strategy_case = STRATEGY_CASE(RATE_LIMITING);
        success = parse_rate_limiting_sampling_json(response->rate_limiting,
                                                    rate_limiting_json);
    }
    else {
        if (per_operation_json == NULL) {
            fprintf(stderr,
                    "WARNING: JSON response contains no strategies, response = "
                    "\"%s\"\n",
                    manager->response_buffer);
            success = false;
            goto cleanup;
        }

        response->strategy_case = STRATEGY_CASE(PER_OPERATION);
        success = parse_per_operation_sampling_json(response->per_operation,
                                                    per_operation_json);
    }

#undef STRATEGY_CASE

cleanup:
    json_decref(response_json);
    if (probabilistic_json != NULL) {
        json_decref(probabilistic_json);
    }
    if (rate_limiting_json != NULL) {
        json_decref(rate_limiting_json);
    }
    if (per_operation_json != NULL) {
        json_decref(per_operation_json);
    }
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

    const int num_written = write(
        manager->fd, &manager->request_buffer[0], manager->request_length);
    if (num_written != manager->request_length) {
        fprintf(stderr,
                "ERROR: Cannot write entire HTTP sampling request, "
                "num written = %d, request length = %d\n",
                num_written,
                manager->request_length);
        return false;
    }

    char chunk_buffer[JAEGERTRACINGC_HTTP_SAMPLING_MANAGER_REQUEST_MAX_LEN];
    int num_read = read(manager->fd, &chunk_buffer[0], sizeof(chunk_buffer));
    while (num_read > 0) {
        const int num_parsed = http_parser_execute(
            &manager->parser, &manager->settings, &chunk_buffer[0], num_read);
        if (num_parsed != num_read) {
            return false;
        }
        num_read = read(manager->fd, &chunk_buffer[0], sizeof(chunk_buffer));
    }

    if (manager->response_buffer != NULL) {
        manager->response_buffer[manager->response_length] = '\0';
        /* Do not increase length because string should not contain null byte.
         */
    }

    return jaeger_http_sampling_manager_parse_json_response(manager, response);
}

static inline void
jaeger_http_sampling_manager_destroy(jaeger_http_sampling_manager* manager)
{
    assert(manager != NULL);
    close(manager->fd);
    if (manager->response_buffer != NULL) {
        jaeger_free(manager->response_buffer);
    }
}

static bool
jaeger_remotely_controlled_sampler_is_sampled(jaeger_sampler* sampler,
                                              const jaeger_trace_id* trace_id,
                                              const char* operation_name,
                                              jaeger_tag_list* tags)
{
    /* TODO */
    return true;
}

static void jaeger_remotely_controlled_sampler_close(jaeger_sampler* sampler)
{
    jaeger_remotely_controlled_sampler* s =
        (jaeger_remotely_controlled_sampler*) sampler;
    jaeger_ticker_destroy(&s->ticker);
    jaeger_sampler_choice* sampler_choice = &s->sampler;

#define SAMPLER_TYPE_CASE(type)                                 \
    case jaeger_##type##_sampler_type:                          \
        sampler_choice->type##_sampler.close(                   \
            (jaeger_sampler*) &sampler_choice->type##_sampler); \
        break

    switch (sampler_choice->type) {
        SAMPLER_TYPE_CASE(const);
        SAMPLER_TYPE_CASE(probabilistic);
        SAMPLER_TYPE_CASE(rate_limiting);
        SAMPLER_TYPE_CASE(guaranteed_throughput_probabilistic);
        SAMPLER_TYPE_CASE(adaptive);
    default:
        fprintf(stderr,
                "ERROR: Invalid sampler type in sampler choice, sampler type = "
                "%d\n",
                sampler_choice->type);
        break;
    }

#undef SAMPLER_TYPE_CASE

    jaeger_http_sampling_manager_destroy(&s->manager);
}

static void jaeger_remotely_controlled_sampler_update(void* context)
{
    assert(context != NULL);
    jaeger_remotely_controlled_sampler* sampler =
        (jaeger_remotely_controlled_sampler*) context;
    jaeger_strategy_response response;
    const bool result = jaeger_http_sampling_manager_get_sampling_strategies(
        &sampler->manager, &response);
    if (result) {
        /* TODO */
    }
}

static const jaeger_duration default_sampling_refresh_interval = {60, 0};

#define DEFAULT_MAX_OPERATIONS 2000
#define DEFAULT_SAMPLING_RATE 0.001

bool jaeger_remotely_controlled_sampler_init(
    jaeger_remotely_controlled_sampler* sampler,
    const char* service_name,
    const char* sampling_server_url,
    const jaeger_sampler_choice* initial_sampler,
    int max_operations,
    const jaeger_duration* sampling_refresh_interval,
    jaeger_metrics* metrics)
{
    assert(sampler != NULL);
    assert(service_name != NULL);
    const jaeger_duration interval = (sampling_refresh_interval != NULL &&
                                      (sampling_refresh_interval->tv_sec > 0 ||
                                       sampling_refresh_interval->tv_nsec > 0))
                                         ? *sampling_refresh_interval
                                         : default_sampling_refresh_interval;
    max_operations =
        (max_operations <= 0) ? DEFAULT_MAX_OPERATIONS : max_operations;
    *sampler = (jaeger_remotely_controlled_sampler){
        &jaeger_remotely_controlled_sampler_is_sampled,
        &jaeger_remotely_controlled_sampler_close,
        (jaeger_sampler_choice){},
        max_operations,
        interval,
        metrics,
        JAEGERTRACINGC_TICKER_INIT,
        (jaeger_http_sampling_manager){
            NULL, NULL, -1, {}, {}, 0, {'\0'}, 0, 0, NULL},
        PTHREAD_MUTEX_INITIALIZER};

    if (initial_sampler != NULL) {
        sampler->sampler = *initial_sampler;
    }
    else {
        sampler->sampler =
            (jaeger_sampler_choice){jaeger_probabilistic_sampler_type};
        jaeger_probabilistic_sampler_init(
            &sampler->sampler.probabilistic_sampler, DEFAULT_SAMPLING_RATE);
    }

    if (!jaeger_http_sampling_manager_init(
            &sampler->manager, sampling_server_url, service_name)) {
        fprintf(stderr,
                "ERROR: Cannot initialize HTTP manager for remotely "
                "controlled sampler\n");
        return false;
    }

    const int result =
        jaeger_ticker_start(&sampler->ticker,
                            &sampler->sampling_refresh_interval,
                            &jaeger_remotely_controlled_sampler_update,
                            &sampler);
    if (result != 0) {
        fprintf(stderr,
                "ERROR: Cannot start ticker in remotely controlled sampler, "
                "return code = %d\n",
                result);
        return false;
    }

    return true;
}
