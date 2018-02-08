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
        jaeger_tag tag = JAEGERTRACINGC_TAG_INIT;
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
    sampler->decision = decision;
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
    const double random_value = ((double) rand_r(&s->seed) + 1) / RAND_MAX;
#else
    const double random_value = ((double) rand() + 1) / RAND_MAX;
#endif /* HAVE_RAND_R */
    const bool decision = (s->sampling_rate >= random_value);
    if (tags != NULL) {
        jaeger_tag tag = JAEGERTRACINGC_TAG_INIT;
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
    sampler->sampling_rate = JAEGERTRACINGC_CLAMP(sampling_rate, 0, 1);
    jaeger_duration duration;
    jaeger_duration_now(&duration);
    sampler->seed = (unsigned int) (duration.tv_nsec >> 32);
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
        jaeger_tag tag = JAEGERTRACINGC_TAG_INIT;
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
    jaeger_token_bucket_init(&sampler->tok,
                             max_traces_per_second,
                             JAEGERTRACINGC_MAX(max_traces_per_second, 1));
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
            jaeger_tag tag = JAEGERTRACINGC_TAG_INIT;
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
        jaeger_tag tag = JAEGERTRACINGC_TAG_INIT;
        tag.key = JAEGERTRACINGC_SAMPLER_TYPE_TAG_KEY;
        tag.value_case = JAEGERTRACING__PROTOBUF__TAG__VALUE_STR_VALUE;
        tag.str_value = JAEGERTRACINGC_SAMPLER_TYPE_LOWER_BOUND;
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

    if (*num_op_samplers == 0) {
        return NULL;
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
    if (s->op_samplers != NULL) {
        for (int i = 0; i < s->num_op_samplers; i++) {
            jaeger_operation_sampler_destroy(&s->op_samplers[i]);
        }
        jaeger_free(s->op_samplers);
        s->op_samplers = NULL;
        s->num_op_samplers = 0;
        s->op_samplers_capacity = 0;
    }
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
    if (sampler->num_op_samplers > 0 && sampler->op_samplers == NULL) {
        return false;
    }
    sampler->op_samplers_capacity = sampler->num_op_samplers;
    sampler->max_operations = max_operations;
    sampler->mutex = (pthread_mutex_t) PTHREAD_MUTEX_INITIALIZER;
    sampler->is_sampled = &jaeger_adaptive_sampler_is_sampled;
    sampler->close = &jaeger_adaptive_sampler_close;
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

        /* Only consider allocating new samplers if we have not had issues with
         * memory. Otherwise, success will be false and we will skip this part
         * entirely.
         */
        if (!found_match && success) {
            if (sampler->num_op_samplers == sampler->op_samplers_capacity &&
                !jaeger_adaptive_sampler_resize_op_samplers(sampler)) {
                fprintf(stderr,
                        "ERROR: Cannot allocate more operation samplers "
                        "in adaptive sampler\n");
                success = false;
                /* Continue with loop so we can update existing samplers despite
                 * memory issues. */
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
                jaeger_free(op_sampler);
                success = false;
                /* Continue with loop so we can update existing samplers despite
                 * memory issues. */
                continue;
            }
            sampler->num_op_samplers++;
        }
    }
    pthread_mutex_unlock(&sampler->mutex);
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
    if (url->parts.field_set & UF_PATH) {
        path_segment = (str_segment){.off = url->parts.field_data[UF_PATH].off,
                                     .len = url->parts.field_data[UF_PATH].len};
    }
    int path_len = (path_segment.len > 0) ? path_segment.len + 1 : 2;
    char path_buffer[path_len];
    path_buffer[path_len] = '\0';
    if (path_segment.len > 0) {
        memcpy(&path_buffer[0], &url->str[path_segment.off], path_segment.len);
    }
    else {
        path_buffer[0] = '/';
    }

    char host_port_buffer[256];
    int result = jaeger_host_port_format(
        sampling_host_port, &host_port_buffer[0], sizeof(host_port_buffer));
    if (result > sizeof(host_port_buffer)) {
        fprintf(
            stderr,
            "ERROR: Cannot write entire sampling server host port to buffer, "
            "buffer size = %zu, host port string length = %d\n",
            sizeof(host_port_buffer),
            result);
        return false;
    }

    result = snprintf(&manager->request_buffer[0],
                      sizeof(manager->request_buffer),
                      "GET %s?service=%s\r\n"
                      "Host: %s\r\n"
                      "User-Agent: jaegertracing/%s\r\n\r\n",
                      &path_buffer[0],
                      manager->service_name,
                      &host_port_buffer[0],
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
jaeger_http_sampling_manager_connect(jaeger_http_sampling_manager* manager,
                                     const jaeger_host_port* sampling_host_port)
{
    struct addrinfo* host_addrs = NULL;
    if (!jaeger_host_port_resolve(sampling_host_port, &host_addrs)) {
        return false;
    }

    bool success = false;
    char ip_str[INET_ADDRSTRLEN];
    for (struct addrinfo* addr_iter = host_addrs; addr_iter != NULL;
         addr_iter = addr_iter->ai_next) {
        const int fd = socket(AF_INET, SOCK_STREAM, 0);
        if (fd < 0) {
            continue;
        }

        if (addr_iter->ai_addrlen == sizeof(struct sockaddr_in)) {
            const struct sockaddr_in* ip_addr =
                (const struct sockaddr_in*) addr_iter->ai_addr;
            const char* result = inet_ntop(
                AF_INET, &ip_addr->sin_addr, &ip_str[0], sizeof(ip_str));
            if (result != NULL) {
                printf("Attempting to connect to %s:%d\n",
                       ip_str,
                       ntohs(ip_addr->sin_port));
            }
        }
        if (connect(fd, addr_iter->ai_addr, addr_iter->ai_addrlen) == 0) {
            manager->fd = fd;
            success = true;
            break;
        }

        close(fd);
    }
    if (!success) {
        fprintf(stderr,
                "ERROR: Cannot connect to sampling server URL, URL = \"%s\"\n",
                manager->sampling_server_url.str);
    }
    freeaddrinfo(host_addrs);
    return success;
}

static inline void
jaeger_http_sampling_manager_destroy(jaeger_http_sampling_manager* manager)
{
    assert(manager != NULL);
    if (manager->fd >= 0) {
        close(manager->fd);
    }
    if (manager->response_buffer != NULL) {
        jaeger_free(manager->response_buffer);
        manager->response_buffer = NULL;
    }
    jaeger_url_destroy(&manager->sampling_server_url);
    if (manager->service_name != NULL) {
        jaeger_free(manager->service_name);
        manager->service_name = NULL;
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
        fprintf(stderr,
                "ERROR: Cannot allocate service name for HTTP sampling "
                "manager, service name = \"%s\"\n",
                service_name);
        return false;
    }

    sampling_server_url =
        (sampling_server_url != NULL && strlen(sampling_server_url) > 0)
            ? sampling_server_url
            : "http://localhost:5778/sampling";
    jaeger_host_port sampling_host_port = {.host = NULL, .port = 0};
    if (!jaeger_url_init(&manager->sampling_server_url, sampling_server_url) ||
        !jaeger_host_port_from_url(&sampling_host_port,
                                   &manager->sampling_server_url) ||
        !jaeger_http_sampling_manager_connect(manager, &sampling_host_port)) {
        jaeger_host_port_destroy(&sampling_host_port);
        jaeger_http_sampling_manager_destroy(manager);
        return false;
    }

    http_parser_init(&manager->parser, HTTP_RESPONSE);
    manager->parser.data = manager;
    manager->settings.on_headers_complete =
        &jaeger_http_sampling_manager_parser_on_headers_complete;
    manager->settings.on_body = &jaeger_http_sampling_manager_parser_on_body;

    manager->response_buffer = jaeger_malloc(RESPONSE_BUFFER_INIT_SIZE);
    if (manager->response_buffer == NULL) {
        fprintf(stderr,
                "ERROR: Cannot allocate response buffer in HTTP sampling "
                "manager, size = %d\n",
                RESPONSE_BUFFER_INIT_SIZE);
        jaeger_host_port_destroy(&sampling_host_port);
        jaeger_http_sampling_manager_destroy(manager);
        return false;
    }

    manager->response_length = 0;
    manager->response_capacity = RESPONSE_BUFFER_INIT_SIZE;

    const bool decision = jaeger_http_sampling_manager_format_request(
        manager, &manager->sampling_server_url, &sampling_host_port);
    jaeger_host_port_destroy(&sampling_host_port);
    return decision;
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
            fprintf(stderr,
                    "ERROR: Cannot allocate probabilistic strategy, num "
                    "operation "
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
        if (strategy->per_operation_strategy != NULL) {
            for (int i = 0; i < num_allocated; i++) {
                jaeger_operation_strategy* op_strategy =
                    strategy->per_operation_strategy[i];
                if (op_strategy != NULL) {
                    jaeger_operation_strategy_destroy(op_strategy);
                    jaeger_free(op_strategy);
                }
            }
            jaeger_free(strategy->per_operation_strategy);
        }
    }

    json_decref(array);

    return success;
}

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
     * avoid incurring performance hits for multiple allocations, we can
     * keep
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

    if (probabilistic_json != NULL) {
        response->strategy_case =
            JAEGERTRACINGC_STRATEGY_RESPONSE_TYPE(PROBABILISTIC);
        success = parse_probabilistic_sampling_json(response->probabilistic,
                                                    probabilistic_json);
    }
    else if (rate_limiting_json != NULL) {
        response->strategy_case =
            JAEGERTRACINGC_STRATEGY_RESPONSE_TYPE(RATE_LIMITING);
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

        response->strategy_case =
            JAEGERTRACINGC_STRATEGY_RESPONSE_TYPE(PER_OPERATION);
        success = parse_per_operation_sampling_json(response->per_operation,
                                                    per_operation_json);
    }

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
                "num written = %d, request length = %d, errno = %d\n",
                num_written,
                manager->request_length,
                errno);
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
        /* Do not increase length because string should not contain null
         * byte.
         */
    }

    return jaeger_http_sampling_manager_parse_json_response(manager, response);
}

static bool
jaeger_remotely_controlled_sampler_is_sampled(jaeger_sampler* sampler,
                                              const jaeger_trace_id* trace_id,
                                              const char* operation_name,
                                              jaeger_tag_list* tags)
{
    assert(sampler != NULL);
    jaeger_remotely_controlled_sampler* s =
        (jaeger_remotely_controlled_sampler*) sampler;
    pthread_mutex_lock(&s->mutex);
    const bool result = jaeger_sampler_choice_is_sampled(
        &s->sampler, trace_id, operation_name, tags);
    pthread_mutex_unlock(&s->mutex);
    return result;
}

static void jaeger_remotely_controlled_sampler_close(jaeger_sampler* sampler)
{
    jaeger_remotely_controlled_sampler* s =
        (jaeger_remotely_controlled_sampler*) sampler;
    jaeger_sampler_choice* sampler_choice = &s->sampler;
    jaeger_sampler_choice_close(sampler_choice);
    jaeger_http_sampling_manager_destroy(&s->manager);
    pthread_mutex_destroy(&s->mutex);
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
    else {
        jaeger_sampler_choice_close(&sampler->sampler);
        sampler->sampler.type = jaeger_adaptive_sampler_type;
        return jaeger_adaptive_sampler_init(&sampler->sampler.adaptive_sampler,
                                            strategies,
                                            sampler->max_operations);
    }
}

bool jaeger_remotely_controlled_sampler_update(
    jaeger_remotely_controlled_sampler* sampler)
{
    assert(sampler != NULL);
    bool success = true;
    jaeger_strategy_response response = JAEGERTRACINGC_STRATEGY_RESPONSE_INIT;
    const bool result = jaeger_http_sampling_manager_get_sampling_strategies(
        &sampler->manager, &response);
    if (!result) {
        fprintf(stderr,
                "ERROR: Cannot get sampling strategies, will retry later\n");
        success = false;
        if (sampler->metrics != NULL &&
            sampler->metrics->sampler_query_failure) {
            sampler->metrics->sampler_query_failure->inc(
                sampler->metrics->sampler_query_failure, 1);
        }
    }

    pthread_mutex_lock(&sampler->mutex);

    if (sampler->metrics && sampler->metrics->sampler_retrieved) {
        sampler->metrics->sampler_retrieved->inc(
            sampler->metrics->sampler_retrieved, 1);
    }

    switch (response.strategy_case) {
    case JAEGERTRACINGC_STRATEGY_RESPONSE_TYPE(PER_OPERATION): {
        if (!jaeger_remotely_controlled_sampler_update_adaptive_sampler(
                sampler, response.per_operation)) {
            fprintf(stderr,
                    "ERROR: Cannot update adaptive sampler in remotely "
                    "controlled "
                    "sampler\n");
        }
    } break;
    case JAEGERTRACINGC_STRATEGY_RESPONSE_TYPE(PROBABILISTIC): {
        if (response.probabilistic == NULL) {
            fprintf(stderr, "ERROR: Received null probabilistic strategy\n");
        }
        else {
            jaeger_sampler_choice_close(&sampler->sampler);
            sampler->sampler.type = jaeger_probabilistic_sampler_type;
            sampler->sampler.probabilistic_sampler.sampling_rate =
                JAEGERTRACINGC_CLAMP(
                    response.probabilistic->sampling_rate, 0, 1);
        }
    } break;
    default: {
        if (response.strategy_case !=
            JAEGERTRACINGC_STRATEGY_RESPONSE_TYPE(RATE_LIMITING)) {
            fprintf(stderr,
                    "ERROR: Invalid strategy type in response, type = %d\n",
                    response.strategy_case);
        }
        else if (response.rate_limiting == NULL) {
            fprintf(stderr, "ERROR: Received null rate limiting strategy\n");
        }
        else {
            jaeger_sampler_choice_close(&sampler->sampler);
            int max_traces_per_second =
                response.rate_limiting->max_traces_per_seconds;
            sampler->sampler.type = jaeger_rate_limiting_sampler_type;
            jaeger_rate_limiting_sampler_init(
                &sampler->sampler.rate_limiting_sampler, max_traces_per_second);
        }
    } break;
    }

    pthread_mutex_unlock(&sampler->mutex);
    jaeger_strategy_response_destroy(&response);
    return success;
}

#define DEFAULT_MAX_OPERATIONS 2000
#define DEFAULT_SAMPLING_RATE 0.001

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
        &jaeger_remotely_controlled_sampler_is_sampled,
        &jaeger_remotely_controlled_sampler_close,
        (jaeger_sampler_choice){},
        max_operations,
        metrics,
        JAEGERTRACINGC_HTTP_SAMPLING_MANAGER_INIT,
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

    return true;
}
