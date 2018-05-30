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

#define HTTP_STATUS_CODES(X)           \
    X(200, ok, "OK")                   \
    X(400, bad_request, "Bad Request") \
    X(404, not_found, "Not Found")     \
    X(500, internal_server_error, "Internal Server Error")

typedef enum http_status_code {
#define X(num, name, str) http_status_code_##name = num,
    HTTP_STATUS_CODES(X)
#undef X
} http_status_code;

static inline const char* status_line(const http_status_code code)
{
#define X(code, _, str) \
    case (code):        \
        return (str);

    switch (code) {
        HTTP_STATUS_CODES(X)
    default:
        assert(false);
    }

#undef X
}

static inline http_status_code
json_obj_to_hashtable(jaeger_hashtable* hashtable, json_t* json)
{
    if (hashtable == NULL || json == NULL || !json_is_object(json)) {
        return http_status_code_bad_request;
    }
    if (!jaeger_hashtable_init(hashtable)) {
        return http_status_code_internal_server_error;
    }

    http_status_code code = http_status_code_ok;
    const char* key;
    json_t* value;
    json_object_foreach(json, key, value)
    {
        if (!json_is_string(value)) {
            fprintf(stderr, "Invalid value: ");
            json_dumpf(value, stderr, 0);
            fprintf(stderr, "\n");
            code = http_status_code_bad_request;
            goto cleanup;
        }
        if (!jaeger_hashtable_put(hashtable, key, json_string_value(value))) {
            fprintf(stderr, "Not enough memory for hashtable, object = ");
            json_dumpf(value, stderr, 0);
            fprintf(stderr, "\n");
            code = http_status_code_internal_server_error;
            goto cleanup;
        }
    }

    return code;

cleanup:
    jaeger_hashtable_destroy(hashtable);
    return code;
}

typedef enum transport_type {
    transport_type_http,
    transport_type_tchannel,
    transport_type_dummy
} transport_type;

static inline http_status_code
parse_transport_type(const char* transport_type_name, transport_type* result)
{
    if (result == NULL || transport_type_name == NULL ||
        strlen(transport_type_name) == 0) {
        return http_status_code_bad_request;
    }

#define TRANSPORT_CMP(name, type)                       \
    do {                                                \
        if (strcmp(transport_type_name, (name)) == 0) { \
            *result = transport_type_##type;            \
            return http_status_code_ok;                 \
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
    return http_status_code_bad_request;

#undef TRANSPORT_CMP

    return http_status_code_bad_request;
}

#define ERR_FMT                                                   \
    "message = \"%s\", source = \"%s\", line = %d, column = %d, " \
    "position = %d\n"

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

static inline http_status_code parse_downstream_message(
    downstream_message* downstream, json_t* json, const char* source)
{
    if (downstream == NULL || json == NULL) {
        return http_status_code_bad_request;
    }
    json_error_t err;
    downstream_message tmp;
    json_t* downstream_child = NULL;
    const int result = json_unpack_ex(json,
                                      &err,
                                      0,
                                      "{ss ss ss ss ss s?o}",
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
        return http_status_code_bad_request;
    }

    http_status_code code = http_status_code_ok;
    *downstream = (downstream_message){.service_name = strdup(tmp.service_name),
                                       .server_role = strdup(tmp.server_role),
                                       .host = strdup(tmp.host),
                                       .port = strdup(tmp.port),
                                       .transport = tmp.transport,
                                       .downstream = NULL};
    if (downstream_child != NULL) {
        code = parse_downstream_message(
            downstream->downstream, downstream_child, source);
        if (code != http_status_code_ok) {
            goto cleanup;
        }
    }

    if (downstream->service_name == NULL || downstream->server_role == NULL ||
        downstream->host == NULL || downstream->port == NULL) {
        code = http_status_code_internal_server_error;
        goto cleanup;
    }

    return code;

cleanup:
    downstream_message_destroy(downstream);
    return code;
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

static inline http_status_code parse_start_trace_request(
    start_trace_request* req, json_t* json, const char* source)
{
    if (req == NULL || json == NULL) {
        return http_status_code_bad_request;
    }

    json_error_t err;
    char* server_role;
    int sampled;
    json_t* baggage;
    json_t* downstream;
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
        return http_status_code_bad_request;
    }
    *req = (start_trace_request){.server_role = strdup(server_role),
                                 .sampled = (sampled != 0),
                                 .baggage = JAEGERTRACINGC_HASHTABLE_INIT,
                                 .downstream = {0}};
    http_status_code code = http_status_code_ok;
    if (req->server_role == NULL) {
        code = http_status_code_internal_server_error;
        goto cleanup;
    }
    code = json_obj_to_hashtable(&req->baggage, baggage);
    if (code != http_status_code_ok) {
        goto cleanup;
    }
    code = parse_downstream_message(&req->downstream, downstream, source);
    if (code != http_status_code_ok) {
        goto cleanup;
    }

    assert(code == http_status_code_ok);
    return code;

cleanup:
    start_trace_request_destroy(req);
    return code;
}

static inline void start_trace(json_t* json, const char* source, int client_fd)
{
    /* TODO */
    (void) client_fd;
    start_trace_request req;
    http_status_code code = parse_start_trace_request(&req, json, source);
    if (code != http_status_code_ok) {
        goto cleanup;
    }

    enum { buffer_size = 512 };
    char buffer[buffer_size];
    opentracing_tracer* tracer = opentracing_global_tracer();
    if (tracer == NULL) {
        code = http_status_code_internal_server_error;
        goto cleanup;
    }

cleanup:
    do {
        const int len = snprintf(buffer,
                                 sizeof(buffer) - 1,
                                 "HTTP/1.1 %d %s\r\n\r\n",
                                 code,
                                 status_line(code));
        (void) write(client_fd, buffer, len);
        close(client_fd);
    } while (0);
}

static inline void join_trace(json_t* json, const char* source, int client_fd)
{
    /* TODO */
    (void) json;
    (void) client_fd;
}

static inline int parse_content_length(const char* request)
{
    int value = 0;
    const char* line = strstr(request, "Content-Length: ");
    if (line != NULL) {
        sscanf(line, "Content-Length: %d", &value);
    }
    return value;
}

static inline void handle_request(const char* request, int client_fd)
{
#define SHORT_SIZE 64
#define STR_(X) #X
#define STR(X) STR_(X)
#define SHORT_FMT STR(SHORT_SIZE) "s"

    char method[SHORT_SIZE];
    char path[SHORT_SIZE];
    sscanf(request, "%" SHORT_FMT " %" SHORT_FMT " HTTP/1.1\r\n", method, path);
    http_status_code code = http_status_code_ok;
    enum { body_size = 512 };
    char body[body_size];
    const char* body_pos = strstr(request, "\r\n\r\n");
    if (body_pos != NULL) {
        body_pos += strlen("\r\n\r\n");
        const int content_length = parse_content_length(request);
        if (content_length == 0) {
            strncpy(body, body_pos, sizeof(body) - 2);
            body[sizeof(body) - 1] = '\0';
        }
        else {
            if (sizeof(body) - 1 < content_length) {
                code = http_status_code_internal_server_error;
                goto cleanup;
            }
            strncpy(body, body_pos, content_length);
            body[content_length + 1] = '\0';
        }
    }

    printf("path = %s\n", path);
    if (strcmp(path, "/start_trace") == 0 || strcmp(path, "/join_trace") == 0) {
        if (body_pos == NULL) {
            code = http_status_code_bad_request;
            goto cleanup;
        }
        json_error_t err;
        json_t* json_body = json_loads(body, 0, &err);
        if (json_body == NULL) {
            PRINT_ERR_MSG(body);
            code = http_status_code_bad_request;
            goto cleanup;
        }

        if (strcmp(path, "/start_trace") == 0) {
            start_trace(json_body, body, client_fd);
        }
        else {
            assert(strcmp(path, "/join_trace") == 0);
            join_trace(json_body, body, client_fd);
        }
        free(json_body);
        close(client_fd);
        return;
    }
    else if (strcmp(path, "/") != 0) {
        code = http_status_code_not_found;
    }

cleanup:
    do {
        enum { buffer_size = 64 };
        char buffer[buffer_size];
        const int len = snprintf(buffer,
                                 sizeof(buffer),
                                 "HTTP/1.1 %d %s\r\n\r\n",
                                 code,
                                 status_line(code));
        (void) write(client_fd, buffer, len);
        close(client_fd);
    } while (0);
}

static void* serve(void* arg)
{
    enum {
        buffer_size = 1024,
    };
    const int client_fd = *(const int*) arg;
    char buffer[buffer_size];
    const int num_read = read(client_fd, buffer, buffer_size - 1);
    if (num_read + 1 >= buffer_size) {
        http_status_code code = http_status_code_internal_server_error;
        fprintf(stderr, "Request too large for buffer\n");
        const int len = snprintf(buffer,
                                 sizeof(buffer),
                                 "HTTP/1.1 %d %s\r\n\r\n",
                                 code,
                                 status_line(code));
        (void) write(client_fd, buffer, len);
        close(client_fd);
    }
    buffer[num_read + 1] = '\0';

    handle_request(buffer, client_fd);

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
