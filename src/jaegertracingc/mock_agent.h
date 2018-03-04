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
 * Key value pair representation.
 */

#ifndef JAEGERTRACINGC_MOCK_AGENT_H
#define JAEGERTRACINGC_MOCK_AGENT_H

#include <errno.h>
#include <http_parser.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include "jaegertracingc/common.h"
#include "jaegertracingc/sampler.h"
#include "jaegertracingc/threading.h"
#include "jaegertracingc/vector.h"
#include "unity.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define MOCK_HTTP_BACKLOG 1
#define MOCK_HTTP_MAX_URL 256

typedef struct mock_http_server {
    int server_fd;
    int client_fd;
    struct sockaddr_in addr;
    jaeger_thread thread;
    jaeger_mutex mutex;
    int url_len;
    char url_buffer[MOCK_HTTP_MAX_URL];
    jaeger_vector responses;
    bool running;
} mock_http_server;

typedef struct mock_http_response {
    char* service_name;
    char* json_format;
    double arg_value;
} mock_http_response;

#define MOCK_HTTP_SERVER_INIT                                          \
    {                                                                  \
        .server_fd = -1, .addr = {}, .thread = 0,                      \
        .mutex = JAEGERTRACINGC_MUTEX_INIT, .url_len = 0,              \
        .url_buffer = {'\0'}, .responses = JAEGERTRACINGC_VECTOR_INIT, \
        .running = false                                               \
    }

static inline int
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

static inline bool
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
        num_read = read(client_fd,
                        buffer,
                        JAEGERTRACINGC_HTTP_SAMPLING_MANAGER_REQUEST_MAX_LEN);
    }

    return (num_read >= 0);
}

typedef struct mock_http_server_context {
    mock_http_server* server;
    jaeger_cond cv;
} mock_http_server_context;

#define MOCK_HTTP_SERVER_CONTEXT_INIT                  \
    {                                                  \
        .server = NULL, .cv = JAEGERTRACINGC_COND_INIT \
    }

static inline void* mock_http_server_run_loop(void* context)
{
    TEST_ASSERT_NOT_NULL(context);
    mock_http_server_context* server_context =
        (mock_http_server_context*) context;
    mock_http_server* server = server_context->server;
    TEST_ASSERT_NOT_NULL(server);
    server->client_fd = -1;

    jaeger_mutex_lock(&server->mutex);
    server->running = true;
    jaeger_mutex_unlock(&server->mutex);
    jaeger_cond_signal(&server_context->cv);

    const int client_fd = accept(server->server_fd, NULL, 0);
    TEST_ASSERT_GREATER_OR_EQUAL(0, client_fd);

    jaeger_mutex_lock(&server->mutex);
    server->client_fd = client_fd;
    jaeger_mutex_unlock(&server->mutex);

    http_parser parser;
    http_parser_init(&parser, HTTP_REQUEST);
    parser.data = server;
    http_parser_settings settings;
    memset(&settings, 0, sizeof(settings));
    settings.on_url = &mock_http_server_on_url;

    while (true) {
        server->url_len = 0;
        char buffer[JAEGERTRACINGC_HTTP_SAMPLING_MANAGER_REQUEST_MAX_LEN];
        int buffer_len = 0;
        if (!read_client_request(server->client_fd, buffer, &buffer_len) ||
            buffer_len == 0) {
            break;
        }

        const int parsed_len =
            http_parser_execute(&parser, &settings, buffer, buffer_len);
        TEST_ASSERT_EQUAL(buffer_len, parsed_len);
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

        const mock_http_response* response = NULL;
        jaeger_mutex_lock(&server->mutex);
        for (int i = 0, len = jaeger_vector_length(&server->responses); i < len;
             i++) {
            const mock_http_response* existing_response =
                (const mock_http_response*) jaeger_vector_offset(
                    &server->responses, i);
            TEST_ASSERT_NOT_NULL(existing_response);
            TEST_ASSERT_NOT_NULL(existing_response->service_name);
            char buffer[strlen("service=") +
                        strlen(existing_response->service_name) + 1];
            TEST_ASSERT_EQUAL(sizeof(buffer) - 1,
                              snprintf(buffer,
                                       sizeof(buffer),
                                       "service=%s",
                                       existing_response->service_name));
            if (strcmp(buffer, query) == 0) {
                response = existing_response;
                break;
            }
        }
        TEST_ASSERT_NOT_NULL(response);

        char strategy_response[strlen(response->json_format) + 10];
        TEST_ASSERT_LESS_THAN(sizeof(strategy_response),
                              snprintf(strategy_response,
                                       sizeof(strategy_response),
                                       response->json_format,
                                       response->arg_value));
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
        jaeger_mutex_unlock(&server->mutex);

        const int num_written =
            write(server->client_fd, http_response, strlen(http_response));
        TEST_ASSERT_EQUAL(strlen(http_response), num_written);
    }
    return NULL;
}

static inline void mock_http_response_destroy(mock_http_response* response)
{
    TEST_ASSERT_NOT_NULL(response);
    TEST_ASSERT_NOT_NULL(response->service_name);
    TEST_ASSERT_NOT_NULL(response->json_format);
    jaeger_free(response->service_name);
    response->service_name = NULL;
    jaeger_free(response->json_format);
    response->json_format = NULL;
    response->arg_value = 0;
}

static inline void mock_http_response_copy(mock_http_response* restrict dst,
                                           const mock_http_response* src)
{
    dst->service_name = jaeger_strdup(src->service_name);
    TEST_ASSERT_NOT_NULL(dst->service_name);
    dst->json_format = jaeger_strdup(src->json_format);
    TEST_ASSERT_NOT_NULL(dst->json_format);
    dst->arg_value = src->arg_value;
}

static inline void
mock_http_server_set_response(mock_http_server* server,
                              const mock_http_response* response)
{
    TEST_ASSERT_NOT_NULL(server);
    TEST_ASSERT_NOT_NULL(response);
    TEST_ASSERT_NOT_NULL(response->service_name);
    TEST_ASSERT_NOT_NULL(response->json_format);

    jaeger_mutex_lock(&server->mutex);
    mock_http_response* response_copy = NULL;
    for (int i = 0, len = jaeger_vector_length(&server->responses); i < len;
         i++) {
        mock_http_response* existing_response =
            (mock_http_response*) jaeger_vector_offset(&server->responses, i);
        TEST_ASSERT_NOT_NULL(existing_response);
        TEST_ASSERT_NOT_NULL(existing_response->service_name);
        if (strcmp(existing_response->service_name, response->service_name) ==
            0) {
            mock_http_response_destroy(existing_response);
            response_copy = existing_response;
        }
    }
    if (response_copy == NULL) {
        response_copy = jaeger_vector_append(&server->responses);
    }
    TEST_ASSERT_NOT_NULL(response_copy);
    mock_http_response_copy(response_copy, response);
    jaeger_mutex_unlock(&server->mutex);
}

static inline void mock_http_server_start(mock_http_server* server)
{
    TEST_ASSERT_NOT_NULL(server);

    TEST_ASSERT_TRUE(
        jaeger_vector_init(&server->responses, sizeof(mock_http_response)));

    server->server_fd = socket(AF_INET, SOCK_STREAM, 0);
    TEST_ASSERT_GREATER_OR_EQUAL(0, server->server_fd);
    server->addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server->addr.sin_family = AF_INET;
    server->addr.sin_port = 0;
    TEST_ASSERT_EQUAL(0,
                      bind(server->server_fd,
                           (struct sockaddr*) &server->addr,
                           sizeof(server->addr)));
    TEST_ASSERT_EQUAL(0, listen(server->server_fd, MOCK_HTTP_BACKLOG));
    socklen_t addr_len = sizeof(server->addr);
    TEST_ASSERT_EQUAL(0,
                      getsockname(server->server_fd,
                                  (struct sockaddr*) &server->addr,
                                  &addr_len));
    TEST_ASSERT_EQUAL(sizeof(server->addr), addr_len);

    mock_http_server_context context = MOCK_HTTP_SERVER_CONTEXT_INIT;
    context.server = server;

    jaeger_mutex_lock(&server->mutex);
    TEST_ASSERT_EQUAL(0,
                      jaeger_thread_init(&server->thread,
                                         &mock_http_server_run_loop,
                                         &context));
    while (!server->running) {
        jaeger_cond_wait(&context.cv, &server->mutex);
    }
    jaeger_mutex_unlock(&server->mutex);
    jaeger_cond_destroy(&context.cv);
}

static inline void mock_http_server_destroy(mock_http_server* server)
{
    jaeger_mutex_lock(&server->mutex);
    if (server->client_fd > -1) {
        shutdown(server->client_fd, SHUT_RDWR);
        close(server->client_fd);
        server->client_fd = -1;
    }
    if (server->server_fd > -1) {
        shutdown(server->server_fd, SHUT_RDWR);
        close(server->server_fd);
        server->server_fd = -1;
    }
    if (server->thread != 0) {
        server->running = false;
        jaeger_thread_join(server->thread, NULL);
        server->thread = 0;
    }
    jaeger_mutex_unlock(&server->mutex);
    jaeger_mutex_destroy(&server->mutex);
    memset(&server->addr, 0, sizeof(server->addr));
    JAEGERTRACINGC_VECTOR_FOR_EACH(
        &server->responses, mock_http_response_destroy, mock_http_response);
    jaeger_vector_destroy(&server->responses);
}

#ifdef __cplusplus
} /* extern C */
#endif /* __cplusplus */

#endif /* JAEGERTRACINGC_MOCK_AGENT_H */
