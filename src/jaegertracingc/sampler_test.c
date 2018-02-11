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
#include <errno.h>
#include <http_parser.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <stdio.h>
#include <string.h>
#include "unity.h"

#define SET_UP_SAMPLER_TEST()                                      \
    jaeger_tag_list tags;                                          \
    jaeger_tag_list_init(&tags);                                   \
    const char* operation_name = "test-operation";                 \
    (void) operation_name;                                         \
    const jaeger_trace_id trace_id = JAEGERTRACINGC_TRACE_ID_INIT; \
    (void) trace_id

#define TEAR_DOWN_SAMPLER_TEST(sampler)        \
    sampler.close((jaeger_sampler*) &sampler); \
    jaeger_tag_list_destroy(&tags)

#define TEST_DEFAULT_SAMPLING_PROBABILITY 0.5

#define CHECK_TAGS(                                                          \
    sampler_type, param_type, value_member, param_value, tag_list)           \
    do {                                                                     \
        TEST_ASSERT_EQUAL(2, (tag_list).size);                               \
        TEST_ASSERT_EQUAL_STRING(JAEGERTRACINGC_SAMPLER_TYPE_TAG_KEY,        \
                                 (tag_list).tags[0].key);                    \
        TEST_ASSERT_EQUAL(JAEGERTRACINGC_TAG_TYPE(STR),                      \
                          (tag_list).tags[0].value_case);                    \
        TEST_ASSERT_EQUAL_STRING(JAEGERTRACINGC_SAMPLER_TYPE_##sampler_type, \
                                 (tag_list).tags[0].str_value);              \
        TEST_ASSERT_EQUAL_STRING(JAEGERTRACINGC_SAMPLER_PARAM_TAG_KEY,       \
                                 (tag_list).tags[1].key);                    \
        TEST_ASSERT_EQUAL(JAEGERTRACINGC_TAG_TYPE(param_type),               \
                          (tag_list).tags[1].value_case);                    \
        TEST_ASSERT_EQUAL((param_value), (tag_list).tags[1].value_member);   \
    } while (0)

#define CHECK_CONST_TAGS(sampler, tags) \
    CHECK_TAGS(CONST, BOOL, bool_value, (sampler).decision, (tags))

#define CHECK_PROBABILISTIC_TAGS(sampler, tags) \
    CHECK_TAGS(                                 \
        PROBABILISTIC, DOUBLE, double_value, (sampler).sampling_rate, (tags))

#define CHECK_RATE_LIMITING_TAGS(sampler, tags) \
    CHECK_TAGS(RATE_LIMITING,                   \
               DOUBLE,                          \
               double_value,                    \
               (sampler).max_traces_per_second, \
               (tags))

#define CHECK_LOWER_BOUND_TAGS(sampler, tags)                       \
    CHECK_TAGS(LOWER_BOUND,                                         \
               DOUBLE,                                              \
               double_value,                                        \
               (sampler).lower_bound_sampler.max_traces_per_second, \
               (tags))

void test_const_sampler()
{
    SET_UP_SAMPLER_TEST();

    jaeger_const_sampler c;
    jaeger_const_sampler_init(&c, true);

    TEST_ASSERT_TRUE(
        c.is_sampled((jaeger_sampler*) &c, &trace_id, operation_name, &tags));
    CHECK_CONST_TAGS(c, tags);

    c.close((jaeger_sampler*) &c);
    jaeger_tag_list_clear(&tags);
    jaeger_const_sampler_init(&c, false);
    TEST_ASSERT_FALSE(
        c.is_sampled((jaeger_sampler*) &c, &trace_id, operation_name, &tags));

    TEAR_DOWN_SAMPLER_TEST(c);
}

void test_probabilistic_sampler()
{
    SET_UP_SAMPLER_TEST();

    jaeger_probabilistic_sampler p;
    double sampling_rate = 1;
    jaeger_probabilistic_sampler_init(&p, sampling_rate);
    TEST_ASSERT_TRUE(
        p.is_sampled((jaeger_sampler*) &p, &trace_id, operation_name, &tags));
    CHECK_PROBABILISTIC_TAGS(p, tags);
    p.close((jaeger_sampler*) &p);

    sampling_rate = 0;
    jaeger_tag_list_clear(&tags);
    jaeger_probabilistic_sampler_init(&p, sampling_rate);
    TEST_ASSERT_FALSE(
        p.is_sampled((jaeger_sampler*) &p, &trace_id, operation_name, &tags));
    CHECK_PROBABILISTIC_TAGS(p, tags);

    TEAR_DOWN_SAMPLER_TEST(p);
}

void test_rate_limiting_sampler()
{
    SET_UP_SAMPLER_TEST();

    const double max_traces_per_second = 1;
    jaeger_rate_limiting_sampler r;
    jaeger_rate_limiting_sampler_init(&r, max_traces_per_second);

    TEST_ASSERT_TRUE(
        r.is_sampled((jaeger_sampler*) &r, &trace_id, operation_name, &tags));
    CHECK_RATE_LIMITING_TAGS(r, tags);

    jaeger_tag_list_clear(&tags);
    TEST_ASSERT_FALSE(
        r.is_sampled((jaeger_sampler*) &r, &trace_id, operation_name, &tags));
    CHECK_RATE_LIMITING_TAGS(r, tags);

    TEAR_DOWN_SAMPLER_TEST(r);
}

void test_guaranteed_throughput_probabilistic_sampler()
{
    SET_UP_SAMPLER_TEST();

    double sampling_rate = TEST_DEFAULT_SAMPLING_PROBABILITY;
    double lower_bound = 2.0;
    jaeger_guaranteed_throughput_probabilistic_sampler g;
    jaeger_guaranteed_throughput_probabilistic_sampler_init(
        &g, lower_bound, sampling_rate);
    TEST_ASSERT_EQUAL(sampling_rate, g.probabilistic_sampler.sampling_rate);
    TEST_ASSERT_EQUAL(lower_bound, g.lower_bound_sampler.max_traces_per_second);

    sampling_rate = 0.6;
    lower_bound = 1.0;
    jaeger_guaranteed_throughput_probabilistic_sampler_update(
        &g, lower_bound, sampling_rate);
    TEST_ASSERT_EQUAL(sampling_rate, g.probabilistic_sampler.sampling_rate);
    TEST_ASSERT_EQUAL(lower_bound, g.lower_bound_sampler.max_traces_per_second);

    sampling_rate = 1.1;
    lower_bound = 1.0;
    jaeger_guaranteed_throughput_probabilistic_sampler_update(
        &g, lower_bound, sampling_rate);
    TEST_ASSERT_EQUAL(1.0, g.probabilistic_sampler.sampling_rate);
    TEST_ASSERT_EQUAL(lower_bound, g.lower_bound_sampler.max_traces_per_second);

    sampling_rate = 0.0;
    jaeger_guaranteed_throughput_probabilistic_sampler_update(
        &g, lower_bound, sampling_rate);
    TEST_ASSERT_TRUE(
        g.is_sampled((jaeger_sampler*) &g, &trace_id, operation_name, &tags));
    CHECK_LOWER_BOUND_TAGS(g, tags);

    jaeger_tag_list_clear(&tags);
    sampling_rate = 1.0;
    jaeger_guaranteed_throughput_probabilistic_sampler_update(
        &g, lower_bound, sampling_rate);
    TEST_ASSERT_TRUE(
        g.is_sampled((jaeger_sampler*) &g, &trace_id, operation_name, &tags));
    CHECK_PROBABILISTIC_TAGS(g.probabilistic_sampler, tags);

    TEAR_DOWN_SAMPLER_TEST(g);
}

#define TEST_DEFAULT_MAX_OPERATIONS 10

void test_adaptive_sampler()
{
    SET_UP_SAMPLER_TEST();

    jaeger_adaptive_sampler a;
    jaeger_per_operation_strategy strategies =
        JAEGERTRACINGC_PER_OPERATION_STRATEGY_INIT;
    strategies.per_operation_strategy =
        jaeger_malloc(sizeof(jaeger_operation_strategy*));
    TEST_ASSERT_NOT_NULL(strategies.per_operation_strategy);
    strategies.n_per_operation_strategy = 1;
    strategies.per_operation_strategy[0] =
        jaeger_malloc(sizeof(jaeger_operation_sampler));
    TEST_ASSERT_NOT_NULL(strategies.per_operation_strategy[0]);
    *strategies.per_operation_strategy[0] =
        (jaeger_operation_strategy) JAEGERTRACINGC_OPERATION_STRATEGY_INIT;
    strategies.per_operation_strategy[0]->operation =
        jaeger_strdup(operation_name);
    TEST_ASSERT_NOT_NULL(strategies.per_operation_strategy[0]->operation);
    strategies.per_operation_strategy[0]->strategy_case =
        JAEGERTRACINGC_OPERATION_STRATEGY_TYPE(PROBABILISTIC);
    strategies.per_operation_strategy[0]->probabilistic =
        jaeger_malloc(sizeof(jaeger_probabilistic_strategy));
    TEST_ASSERT_NOT_NULL(strategies.per_operation_strategy[0]->probabilistic);
    *strategies.per_operation_strategy[0]->probabilistic =
        (jaeger_probabilistic_strategy)
            JAEGERTRACINGC_PROBABILISTIC_STRATEGY_INIT;
    strategies.per_operation_strategy[0]->probabilistic->sampling_rate =
        TEST_DEFAULT_SAMPLING_PROBABILITY;
    strategies.default_lower_bound_traces_per_second = 1.0;
    strategies.default_sampling_probability = TEST_DEFAULT_SAMPLING_PROBABILITY;

    jaeger_adaptive_sampler_init(&a, &strategies, TEST_DEFAULT_MAX_OPERATIONS);
    a.is_sampled((jaeger_sampler*) &a, &trace_id, operation_name, &tags);

    jaeger_tag_list_clear(&tags);
    a.is_sampled((jaeger_sampler*) &a, &trace_id, "new-operation", &tags);
    TEST_ASSERT_EQUAL(2, a.num_op_samplers);
    TEST_ASSERT_NOT_NULL(a.op_samplers[0].operation_name);
    TEST_ASSERT_NOT_NULL(a.op_samplers[1].operation_name);
    TEST_ASSERT_TRUE(strcmp(a.op_samplers[0].operation_name,
                            a.op_samplers[1].operation_name) < 0);

    jaeger_per_operation_strategy_destroy(&strategies);
    TEAR_DOWN_SAMPLER_TEST(a);
}

#define MOCK_HTTP_BACKLOG 1
#define MOCK_HTTP_MAX_URL 256

typedef struct mock_http_server {
    int socket_fd;
    struct sockaddr_in addr;
    pthread_t thread;
    int url_len;
    char url_buffer[MOCK_HTTP_MAX_URL];
} mock_http_server;

#define MOCK_HTTP_SERVER_INIT                                    \
    {                                                            \
        .socket_fd = -1, .addr = {0}, .thread = 0, .url_len = 0, \
        .url_buffer = {                                          \
            '\0'                                                 \
        }                                                        \
    }

static int
mock_http_server_on_url(http_parser* parser, const char* at, size_t len)
{
    TEST_ASSERT_NOT_NULL(parser);
    TEST_ASSERT_NOT_NULL(parser->data);
    mock_http_server* server = (mock_http_server*) parser->data;
    TEST_ASSERT_NOT_NULL(at);
    TEST_ASSERT_LESS_OR_EQUAL(MOCK_HTTP_MAX_URL, server->url_len + len);
    memcpy(&server->url_buffer[server->url_len], at, len);
    server->url_len += len;
    return 0;
}

static inline void
read_client_request(int client_fd, char* buffer, int* buffer_len)
{
    int num_read = read(client_fd,
                        buffer,
                        JAEGERTRACINGC_HTTP_SAMPLING_MANAGER_REQUEST_MAX_LEN);

    while (num_read > 0) {
        TEST_ASSERT_LESS_OR_EQUAL(
            JAEGERTRACINGC_HTTP_SAMPLING_MANAGER_REQUEST_MAX_LEN, *buffer_len);
        TEST_ASSERT_LESS_OR_EQUAL(
            JAEGERTRACINGC_HTTP_SAMPLING_MANAGER_REQUEST_MAX_LEN,
            *buffer_len + num_read);
        *buffer_len += num_read;
        if (*buffer_len >= 4 &&
            memcmp(&buffer[*buffer_len - 4], "\r\n\r\n", 4) == 0) {
            break;
        }
        num_read =
            read(client_fd, &buffer[*buffer_len], sizeof(buffer) - *buffer_len);
    }
}

static void* mock_http_server_run_loop(void* context)
{
    TEST_ASSERT_NOT_NULL(context);
    mock_http_server* server = (mock_http_server*) context;
    int client_fd = -1;

    client_fd = accept(server->socket_fd, NULL, 0);
    http_parser parser;
    http_parser_init(&parser, HTTP_REQUEST);
    parser.data = server;
    http_parser_settings settings;
    memset(&settings, 0, sizeof(settings));
    settings.on_url = &mock_http_server_on_url;

    const char strategy_response_format[] = "{\n"
                                            "  \"probabilisticSampling\": {\n"
                                            "      \"samplingRate\": %f\n"
                                            "    }\n"
                                            "}\n";
    char strategy_response[strlen(strategy_response_format) + 10];
    TEST_ASSERT_LESS_THAN(sizeof(strategy_response),
                          snprintf(strategy_response,
                                   sizeof(strategy_response),
                                   strategy_response_format,
                                   TEST_DEFAULT_SAMPLING_PROBABILITY));
    const char http_response_format[] = "HTTP/1.1 200 OK\r\n"
                                        "Content-Type: application/json\r\n"
                                        "Content-Length: %zu\r\n\r\n"
                                        "%s";
    const int num_digits = 10;
    char http_response[strlen(strategy_response) +
                       strlen(http_response_format) + num_digits + 1];
    const int result = snprintf(http_response,
                                sizeof(http_response),
                                http_response_format,
                                strlen(strategy_response),
                                strategy_response);
    TEST_ASSERT_LESS_OR_EQUAL(sizeof(http_response) - 1, result);

    char buffer[JAEGERTRACINGC_HTTP_SAMPLING_MANAGER_REQUEST_MAX_LEN];
    int buffer_len = 0;
    read_client_request(client_fd, buffer, &buffer_len);

    TEST_ASSERT_EQUAL(
        buffer_len,
        http_parser_execute(&parser, &settings, buffer, buffer_len));
    struct http_parser_url url;
    TEST_ASSERT_EQUAL(
        0,
        http_parser_parse_url(
            &server->url_buffer[0], server->url_len, false, &url));
    TEST_ASSERT_TRUE(url.field_set & (1 << UF_QUERY));
    char query[url.field_data[UF_QUERY].len + 1];
    memcpy(query,
           &server->url_buffer[url.field_data[UF_QUERY].off],
           sizeof(query) - 1);
    query[sizeof(query) - 1] = '\0';
    TEST_ASSERT_EQUAL_STRING("service=test-service", query);
    const int num_written =
        write(client_fd, http_response, strlen(http_response));
    TEST_ASSERT_EQUAL(strlen(http_response), num_written);
    close(client_fd);
    return NULL;
}

static inline void mock_http_server_start(mock_http_server* server)
{
    TEST_ASSERT_NOT_NULL(server);

    server->socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    TEST_ASSERT_GREATER_OR_EQUAL(0, server->socket_fd);
    server->addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    server->addr.sin_family = AF_INET;
    server->addr.sin_port = 0;
    TEST_ASSERT_EQUAL(0,
                      bind(server->socket_fd,
                           (struct sockaddr*) &server->addr,
                           sizeof(server->addr)));
    TEST_ASSERT_EQUAL(0, listen(server->socket_fd, MOCK_HTTP_BACKLOG));
    socklen_t addr_len = sizeof(server->addr);
    TEST_ASSERT_EQUAL(0,
                      getsockname(server->socket_fd,
                                  (struct sockaddr*) &server->addr,
                                  &addr_len));
    TEST_ASSERT_EQUAL(sizeof(server->addr), addr_len);
    TEST_ASSERT_EQUAL(
        0,
        pthread_create(
            &server->thread, NULL, &mock_http_server_run_loop, server));
}

static inline void mock_http_server_destroy(mock_http_server* server)
{
    if (server->socket_fd > -1) {
        shutdown(server->socket_fd, SHUT_RDWR);
        TEST_ASSERT_EQUAL(0, close(server->socket_fd));
        server->socket_fd = -1;
    }
    if (server->thread != 0) {
        TEST_ASSERT_EQUAL(0, pthread_join(server->thread, NULL));
        server->thread = 0;
    }
    memset(&server->addr, 0, sizeof(server->addr));
}

void test_remotely_controlled_sampler()
{
    jaeger_metrics metrics;
    jaeger_null_metrics_init(&metrics);
    mock_http_server server = MOCK_HTTP_SERVER_INIT;
    mock_http_server_start(&server);
    const int port = ntohs(server.addr.sin_port);

#define URL_PREFIX "http://localhost:"
    char buffer[sizeof(URL_PREFIX) + 5];
    const int result = snprintf(buffer, sizeof(buffer), URL_PREFIX "%d", port);
    TEST_ASSERT_LESS_OR_EQUAL(sizeof(buffer) - 1, result);
    jaeger_remotely_controlled_sampler r;
    TEST_ASSERT_TRUE(
        jaeger_remotely_controlled_sampler_init(&r,
                                                "test-service",
                                                buffer,
                                                NULL,
                                                TEST_DEFAULT_MAX_OPERATIONS,
                                                &metrics));
    TEST_ASSERT_TRUE(jaeger_remotely_controlled_sampler_update(&r));
    TEST_ASSERT_EQUAL(jaeger_probabilistic_sampler_type, r.sampler.type);
    TEST_ASSERT_EQUAL(TEST_DEFAULT_SAMPLING_PROBABILITY,
                      r.sampler.probabilistic_sampler.sampling_rate);
    r.close((jaeger_sampler*) &r);
    mock_http_server_destroy(&server);
    jaeger_metrics_destroy(&metrics);
}

void test_sampler_choice()
{
    jaeger_sampler_choice choice;

#define FOR_EACH_SAMPLER_TEST(X)           \
    X(const)                               \
    X(probabilistic)                       \
    X(rate_limiting)                       \
    X(guaranteed_throughput_probabilistic) \
    X(adaptive)

#define CHECK_ASSIGN(sampler_type)                                     \
    do {                                                               \
        choice.type = jaeger_##sampler_type##_sampler_type;            \
        TEST_ASSERT_EQUAL(&choice.sampler_type##_sampler,              \
                          jaeger_sampler_choice_get_sampler(&choice)); \
    } while (0);

    FOR_EACH_SAMPLER_TEST(CHECK_ASSIGN)
}
