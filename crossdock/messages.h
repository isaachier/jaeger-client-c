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

#ifndef CROSSDOCK_MESSAGES_H
#define CROSSDOCK_MESSAGES_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <http_parser.h>
#include <jansson.h>

typedef enum transport_type {
    transport_type_http,
    transport_type_tchannel,
    transport_type_dummy
} transport_type;

typedef struct downstream_message {
    char* service_name;
    char* server_role;
    char* host;
    char* port;
    transport_type transport;
    struct downstream_message* downstream;
} downstream_message;

typedef struct start_trace_request {
    char* server_role;
    bool sampled;
    char* baggage;
    downstream_message downstream;
} start_trace_request;

typedef struct observed_span {
    char* trace_id;
    bool sampled;
    char* baggage;
} observed_span;

static inline const char* http_status_line(enum http_status status)
{
#define CASE(value, id, line) \
    case HTTP_STATUS_##id:    \
        return #line;

    switch (status) {
        HTTP_STATUS_MAP(CASE)
    default:
        assert(false);
    }
}

static inline enum http_status
parse_transport_type(const char* transport_type_name, transport_type* result)
{
    if (result == NULL || transport_type_name == NULL ||
        strlen(transport_type_name) == 0) {
        return HTTP_STATUS_BAD_REQUEST;
    }

#define TRANSPORT_CMP(name, type)                       \
    do {                                                \
        if (strcmp(transport_type_name, (name)) == 0) { \
            *result = transport_type_##type;            \
            return HTTP_STATUS_OK;                      \
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
    return HTTP_STATUS_BAD_REQUEST;

#undef TRANSPORT_CMP

    return HTTP_STATUS_BAD_REQUEST;
}

#define ERR_FMT                                                   \
    "message = \"%s\", source = \"%s\", line = %d, column = %d, " \
    "position = %d\n"

#define ERR_ARGS(source) err.text, (source), err.line, err.column, err.position

#define PRINT_ERR_MSG(source)                       \
    do {                                            \
        fprintf(stderr, ERR_FMT, ERR_ARGS(source)); \
    } while (0)

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

static inline enum http_status parse_downstream_message(
    downstream_message* downstream, json_t* json, const char* source)
{
    if (downstream == NULL || json == NULL) {
        return HTTP_STATUS_BAD_REQUEST;
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
        return HTTP_STATUS_BAD_REQUEST;
    }

    enum http_status code = HTTP_STATUS_OK;
    *downstream = (downstream_message){.service_name = strdup(tmp.service_name),
                                       .server_role = strdup(tmp.server_role),
                                       .host = strdup(tmp.host),
                                       .port = strdup(tmp.port),
                                       .transport = tmp.transport,
                                       .downstream = NULL};
    if (downstream_child != NULL) {
        code = parse_downstream_message(
            downstream->downstream, downstream_child, source);
        if (code != HTTP_STATUS_OK) {
            goto cleanup;
        }
    }

    if (downstream->service_name == NULL || downstream->server_role == NULL ||
        downstream->host == NULL || downstream->port == NULL) {
        code = HTTP_STATUS_INTERNAL_SERVER_ERROR;
        goto cleanup;
    }

    return code;

cleanup:
    downstream_message_destroy(downstream);
    return code;
}

static inline void start_trace_request_destroy(start_trace_request* req)
{
    if (req == NULL) {
        return;
    }
    free(req->server_role);
    free(req->baggage);
    downstream_message_destroy(&req->downstream);
    memset(req, 0, sizeof(*req));
}

static inline enum http_status parse_start_trace_request(
    start_trace_request* req, json_t* json, const char* source)
{
    if (req == NULL || json == NULL) {
        return HTTP_STATUS_BAD_REQUEST;
    }

    json_error_t err;
    char* server_role;
    int sampled;
    char* baggage;
    json_t* downstream;
    const int result = json_unpack_ex(json,
                                      &err,
                                      0,
                                      "{ss sb ss so}",
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
        return HTTP_STATUS_BAD_REQUEST;
    }
    *req = (start_trace_request){.server_role = strdup(server_role),
                                 .sampled = (sampled != 0),
                                 .baggage = strdup(baggage),
                                 .downstream = {0}};
    enum http_status code = HTTP_STATUS_OK;
    if (req->server_role == NULL || req->baggage == NULL) {
        code = HTTP_STATUS_INTERNAL_SERVER_ERROR;
        goto cleanup;
    }
    if (code != HTTP_STATUS_OK) {
        goto cleanup;
    }
    code = parse_downstream_message(&req->downstream, downstream, source);
    if (code != HTTP_STATUS_OK) {
        goto cleanup;
    }

    assert(code == HTTP_STATUS_OK);
    return code;

cleanup:
    start_trace_request_destroy(req);
    return code;
}

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* CROSSDOCK_MESSAGES_H */
