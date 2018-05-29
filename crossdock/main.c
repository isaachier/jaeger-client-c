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

#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <http_parser.h>
#include <jansson.h>
#include <opentracing-c/tracer.h>

#include <jaegertracingc/hashtable.h>

#define ASSERT_PERROR(cond, msg) \
    do {                         \
        if (!(cond)) {           \
            perror(msg);         \
            exit(EXIT_FAILURE);  \
        }                        \
    } while (0)

static inline bool json_obj_to_hashtable(jaeger_hashtable* hashtable,
                                         json_t* json)
{
    if (hashtable == NULL || json == NULL || !json_is_object(json) ||
        !jaeger_hashtable_init(hashtable)) {
        return false;
    }

    const char* key;
    json_t* value;
    json_object_foreach(json, key, value)
    {
        if (!json_is_string(value)) {
            char* value_str = json_dumps(value, 0);
            if (value_str == NULL) {
                fprintf(stderr, "Not enough memory for hashtable\n");
            }
            else {
                fprintf(stderr, "Invalid value: %s\n", value_str);
                free(value_str);
            }
            goto cleanup;
        }
        if (!jaeger_hashtable_put(hashtable, key, json_string_value(value))) {
            fprintf(stderr, "Not enough memory for hashtable\n");
            goto cleanup;
        }
        continue;

    cleanup:
        jaeger_hashtable_destroy(hashtable);
        return false;
    }

    return true;
}

typedef enum transport_type {
    transport_type_http,
    transport_type_tchannel,
    transport_type_dummy
} transport_type;

static inline bool parse_transport_type(const char* transport_type_name,
                                        transport_type* result)
{
    if (result == NULL || transport_type_name == NULL ||
        strlen(transport_type_name) == 0) {
        return false;
    }

#define TRANSPORT_CMP(name, type)                       \
    do {                                                \
        if (strcmp(transport_type_name, (name)) == 0) { \
            *result = transport_type_##type;            \
            return true;                                \
        }                                               \
    } while (0)

    switch (transport_type_name[0]) {
    case 'D':
        TRANSPORT_CMP("DUMMY", dummy);
        break;
    case 'H':
        TRANSPORT_CMP("HTTP", http);
        break;
    case 'T':
        TRANSPORT_CMP("TCHANNEL", tchannel);
        break;
    default:
        break;
    }
    return false;

#undef TRANSPORT_CMP
}

#define ERR_FMT                                                   \
    "message = \"%s\", source = \"%s\", line = %d, column = %d, " \
    "position = %d"

#define ERR_ARGS(source) err.text, (source), err.line, err.column, err.position

#define PRINT_ERR_MSG(source)                       \
    do {                                            \
        fprintf(stderr, ERR_FMT, ERR_ARGS(source)); \
    } while (0)

typedef struct downstream_message {
    char* service_name;
    char* server_role;
    char* host;
    char* port;
    transport_type transport;
    struct downstream_message* downstream;
} downstream_message;

static inline void downstream_message_destroy(downstream_message* downstream)
{
    if (downstream == NULL) {
        return;
    }
    free(downstream->service_name);
    free(downstream->server_role);
    free(downstream->host);
    free(downstream->port);
    free(downstream->downstream);
    memset(downstream, 0, sizeof(*downstream));
}

static inline bool parse_downstream_message(downstream_message* downstream,
                                            json_t* json,
                                            const char* source)
{
    if (downstream == NULL || json == NULL) {
        return false;
    }
    json_error_t err;
    downstream_message tmp;
    json_t* downstream_child = NULL;
    const int result = json_unpack_ex(json,
                                      &err,
                                      0,
                                      "{ss ss ss ss si s?o}",
                                      "serviceName",
                                      &tmp.service_name,
                                      "serverRole",
                                      &tmp.server_role,
                                      "host",
                                      &tmp.host,
                                      "port",
                                      &tmp.port,
                                      "transport",
                                      &tmp.transport,
                                      "downstream",
                                      &downstream_child);

    if (result != 0) {
        PRINT_ERR_MSG(source);
        return false;
    }

    *downstream = (downstream_message){.service_name = strdup(tmp.service_name),
                                       .server_role = strdup(tmp.server_role),
                                       .host = strdup(tmp.host),
                                       .port = strdup(tmp.port),
                                       .transport = tmp.transport,
                                       .downstream = NULL};
    if (downstream_child != NULL &&
        !parse_downstream_message(
            downstream->downstream, downstream_child, source)) {
        goto cleanup;
    }

    if (downstream->service_name == NULL || downstream->server_role == NULL ||
        downstream->host == NULL || downstream->port == NULL) {
        goto cleanup;
    }

    return true;

cleanup:
    downstream_message_destroy(downstream);
    return false;
}

typedef struct start_trace_request {
    char* server_role;
    bool sampled;
    jaeger_hashtable baggage;
    downstream_message downstream;
} start_trace_request;

static inline void start_trace_request_destroy(start_trace_request* req)
{
    if (req == NULL) {
        return;
    }
    free(req->server_role);
    jaeger_hashtable_destroy(&req->baggage);
    downstream_message_destroy(&req->downstream);
    memset(req, 0, sizeof(*req));
}

static inline bool parse_start_trace_request(start_trace_request* req,
                                             json_t* json,
                                             const char* source)
{
    if (req == NULL || json == NULL) {
        return false;
    }

    json_error_t err;
    char* server_role;
    int sampled;
    json_t baggage;
    json_t downstream;
    const int result = json_unpack_ex(json,
                                      &err,
                                      0,
                                      "{ss sb so so}",
                                      "serverRole",
                                      &server_role,
                                      "sampled",
                                      &sampled,
                                      "baggage",
                                      &baggage,
                                      "downstream",
                                      &downstream);
    if (result != 0) {
        PRINT_ERR_MSG(source);
        return false;
    }
    *req = (start_trace_request){.server_role = strdup(server_role),
                                 .sampled = (sampled != 0),
                                 .baggage = JAEGERTRACINGC_HASHTABLE_INIT,
                                 .downstream = {0}};
    if (req->server_role == NULL ||
        !json_obj_to_hashtable(&req->baggage, &baggage) ||
        !parse_downstream_message(&req->downstream, &downstream, source)) {
        start_trace_request_destroy(req);
        return false;
    }
    return true;
}

static void* serve(void* arg)
{
    enum { buffer_size = 1024, path_size = 64 };
    int status_code = 200;
    const char* status_line = "OK";
    const int client_fd = *(const int*) arg;
    char buffer[buffer_size];
    const int num_read = read(client_fd, buffer, buffer_size - 1);
    if (num_read + 1 >= buffer_size) {
        fprintf(stderr, "Request too large for buffer\n");
        goto cleanup;
    }
    buffer[num_read + 1] = '\0';

    char path[path_size];
    sscanf(buffer, "GET %s HTTP/1.1\r\n", path);
    if (strcmp(path, "/start_trace") == 0) {
        printf("START TRACE REQUEST\n");
        /* TODO */
    }
    else if (strcmp(path, "/") != 0) {
        status_code = 404;
        status_line = "Not Found";
    }

cleanup:
    do {
        const int len = snprintf(buffer,
                                 sizeof(buffer),
                                 "HTTP/1.1 %d %s\r\n\r\n",
                                 status_code,
                                 status_line);
        (void) write(client_fd, buffer, len);
        close(client_fd);
    } while (0);
    return NULL;
}

int main(void)
{
    enum { port = 8080, backlog = 8, num_threads = 4 };
    const int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    ASSERT_PERROR(server_fd >= 0, "socket failed");
    struct sockaddr_in addr = {.sin_addr = {.s_addr = htonl(INADDR_ANY)},
                               .sin_port = htons(port)};
    ASSERT_PERROR(bind(server_fd, (struct sockaddr*) &addr, sizeof(addr)) == 0,
                  "bind failed");
    const int enable = 1;
    ASSERT_PERROR(
        setsockopt(
            server_fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable)) == 0,
        "setsockopt failed");
    ASSERT_PERROR(listen(server_fd, backlog) == 0, "listen failed");

    pthread_t threads[num_threads];
    size_t thread_index = 0;
    memset(&threads, 0, sizeof(threads));
    while (true) {
        const int client_fd = accept(server_fd, NULL, 0);
        if (client_fd < 0) {
            perror("accept failed");
            return EXIT_FAILURE;
        }
        pthread_t* thread = &threads[thread_index];
        if (*thread != 0) {
            const int return_code = pthread_join(*thread, NULL);
            ASSERT_PERROR(return_code == 0, "pthread_join failed");
        }
        const int return_code =
            pthread_create(thread, NULL, &serve, (void*) &client_fd);
        ASSERT_PERROR(return_code == 0, "pthread_create failed");
        thread_index = (thread_index + 1) % num_threads;
    }

    return 0;
}
