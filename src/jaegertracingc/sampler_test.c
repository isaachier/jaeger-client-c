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

    jaeger_per_operation_strategy_destroy(&strategies);
    TEAR_DOWN_SAMPLER_TEST(a);
}

#define MOCK_HTTP_BACKLOG 1

typedef struct mock_http_server {
    int socket_fd;
    struct sockaddr_in addr;
    pthread_t thread;
    int done_fd;
} mock_http_server;

#define MOCK_HTTP_SERVER_INIT                     \
    {                                             \
        .socket_fd = -1, .addr = {0}, .thread = 0 \
    }

static void* mock_http_server_run_loop(void* context)
{
#define CHECK_RUNNING()           \
    do {                          \
        if (!running) {           \
            if (client_fd > -1) { \
                close(client_fd); \
            }                     \
            return NULL;          \
        }                         \
    } while (0)

    TEST_ASSERT_NOT_NULL(context);
    mock_http_server* server = (mock_http_server*) context;
    bool running = true;
    int client_fd = -1;
    CHECK_RUNNING();

    client_fd = accept(server->socket_fd, NULL, 0);
    CHECK_RUNNING();
    while (true) {
        char buffer[JAEGERTRACINGC_HTTP_SAMPLING_MANAGER_REQUEST_MAX_LEN];
        int buffer_len = 0;
        int num_read = read(client_fd, &buffer[0], sizeof(buffer));
        CHECK_RUNNING();

        while (num_read > 0) {
            TEST_ASSERT_LESS_OR_EQUAL(
                JAEGERTRACINGC_HTTP_SAMPLING_MANAGER_REQUEST_MAX_LEN,
                buffer_len);
            buffer_len += num_read;
            num_read = read(
                client_fd, &buffer[buffer_len], sizeof(buffer) - buffer_len);
            CHECK_RUNNING();
        }
        char str[buffer_len + 1];
        memcpy(&str[0], &buffer[0], buffer_len);
        str[buffer_len] = '\0';
        printf("HTTP Request:\n%s\n", str);
        break;
    }
    close(client_fd);
    return NULL;

#undef CHECK_RUNNING
}

static inline void mock_http_server_start(mock_http_server* server)
{
    TEST_ASSERT_NOT_NULL(server);

    server->socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    server->addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    server->addr.sin_family = AF_INET;
    server->addr.sin_port = 0;
    TEST_ASSERT_GREATER_OR_EQUAL(0, server->socket_fd);
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
        TEST_ASSERT_EQUAL(0, shutdown(server->socket_fd, SHUT_RDWR));
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
    mock_http_server server = MOCK_HTTP_SERVER_INIT;
    mock_http_server_start(&server);
    const int port = ntohs(server.addr.sin_port);
    printf("Server running on localhost:%d\n", port);
    mock_http_server_destroy(&server);
}
