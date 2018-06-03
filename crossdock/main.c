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
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <jaegertracingc/span.h>
#include <jaegertracingc/trace_id.h>
#include <opentracing-c/tracer.h>

#include "messages.h"

#define ASSERT_PERROR(cond, msg) \
    do {                         \
        if (!(cond)) {           \
            perror(msg);         \
            exit(EXIT_FAILURE);  \
        }                        \
    } while (0)

static const char baggage_key[] = "crossdock-baggage-key";

static inline bool make_observed_span(observed_span* span,
                                      opentracing_span_context* context)
{
    if (context->type_descriptor_length !=
            jaeger_span_context_type_descriptor_length ||
        !strcmp(context->type_descriptor,
                jaeger_span_context_type_descriptor)) {
        fprintf(stderr, "Invalid span context type\n");
        return false;
    }
    jaeger_span_context* ctx = (jaeger_span_context*) context;

    const size_t trace_id_len = JAEGERTRACINGC_TRACE_ID_MAX_STR_LEN + 1;
    span->trace_id = malloc(trace_id_len);
    if (span->trace_id == NULL) {
        return false;
    }
    jaeger_mutex_lock(&ctx->mutex);
    jaeger_trace_id_format(&ctx->trace_id, span->trace_id, trace_id_len);
    span->sampled =
        ((ctx->flags & ((uint8_t) jaeger_sampling_flag_sampled)) != 0);
    const jaeger_key_value* kv =
        jaeger_hashtable_find(&ctx->baggage, baggage_key);
    assert(kv != NULL);
    span->baggage = strdup(kv->value);
    jaeger_mutex_unlock(&ctx->mutex);
    return true;
}

static inline void prepare_response(opentracing_span_context* context,
                                    const char* server_role,
                                    const downstream_message* downstream,
                                    int client_fd)
{
    /* TODO */
    (void) context;
    (void) server_role;
    (void) downstream;
}

static inline void start_trace(json_t* json, const char* source, int client_fd)
{
    /* TODO */
    (void) client_fd;
    start_trace_request req;
    enum http_status code = parse_start_trace_request(&req, json, source);
    if (code != HTTP_STATUS_OK) {
        goto cleanup;
    }

    enum { buffer_size = 512 };
    char buffer[buffer_size];
    opentracing_tracer* tracer = opentracing_global_tracer();
    if (tracer == NULL) {
        code = HTTP_STATUS_INTERNAL_SERVER_ERROR;
        goto cleanup;
    }
    opentracing_span* span = tracer->start_span(tracer, req.server_role);
    if (req.sampled) {
        const opentracing_value value = {
            .type = opentracing_value_uint64,
            .value = {.uint64_value = 1},
        };
        span->set_tag(span, "sampling.priority", &value);
    }
    span->set_baggage_item(span, baggage_key, req.baggage);
    prepare_response(
        span->span_context(span), req.server_role, &req.downstream, client_fd);
    return;

cleanup:
    do {
        const int len = snprintf(buffer,
                                 sizeof(buffer) - 1,
                                 "HTTP/1.1 %d %s\r\n\r\n",
                                 code,
                                 http_status_line(code));
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
    enum http_status code = HTTP_STATUS_OK;
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
                code = HTTP_STATUS_INTERNAL_SERVER_ERROR;
                goto cleanup;
            }
            strncpy(body, body_pos, content_length);
            body[content_length + 1] = '\0';
        }
    }

    printf("path = %s\n", path);
    if (strcmp(path, "/start_trace") == 0 || strcmp(path, "/join_trace") == 0) {
        if (body_pos == NULL) {
            code = HTTP_STATUS_BAD_REQUEST;
            goto cleanup;
        }
        json_error_t err;
        json_t* json_body = json_loads(body, 0, &err);
        if (json_body == NULL) {
            PRINT_ERR_MSG(body);
            code = HTTP_STATUS_BAD_REQUEST;
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
        code = HTTP_STATUS_NOT_FOUND;
    }

cleanup:
    do {
        enum { buffer_size = 64 };
        char buffer[buffer_size];
        const int len = snprintf(buffer,
                                 sizeof(buffer),
                                 "HTTP/1.1 %d %s\r\n\r\n",
                                 code,
                                 http_status_line(code));
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
        enum http_status code = HTTP_STATUS_INTERNAL_SERVER_ERROR;
        fprintf(stderr, "Request too large for buffer\n");
        const int len = snprintf(buffer,
                                 sizeof(buffer),
                                 "HTTP/1.1 %d %s\r\n\r\n",
                                 code,
                                 http_status_line(code));
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
