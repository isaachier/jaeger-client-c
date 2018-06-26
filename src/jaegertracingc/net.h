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
 * Network utilities.
 */

#ifndef JAEGERTRACINGC_NET_H
#define JAEGERTRACINGC_NET_H

#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <http_parser.h>

#include "jaegertracingc/common.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define JAEGERTRACINGC_MAX_PORT_STR_LEN 6

typedef struct jaeger_host_port {
    char* host;
    int port;
} jaeger_host_port;

#define JAEGERTRACINGC_HOST_PORT_INIT \
    {                                 \
        .host = NULL, .port = 0       \
    }

bool jaeger_host_port_init(jaeger_host_port* host_port,
                           const char* host,
                           int port);

void jaeger_host_port_destroy(jaeger_host_port* host_port);

typedef struct jaeger_url {
    char* str;
    struct http_parser_url parts;
} jaeger_url;

#define JAEGERTRACINGC_URL_INIT                   \
    {                                             \
        .str = NULL, .parts = {                   \
            .field_set = 0,                       \
            .field_data = {{.len = 0, .off = -1}} \
        }                                         \
    }

bool jaeger_url_init(jaeger_url* url, const char* url_str);

void jaeger_url_destroy(jaeger_url* url);

bool jaeger_host_port_from_url(jaeger_host_port* host_port,
                               const jaeger_url* url);

bool jaeger_host_port_scan(jaeger_host_port* host_port, const char* str);

int jaeger_host_port_format(const jaeger_host_port* host_port,
                            char* buffer,
                            int buffer_len);

bool jaeger_host_port_resolve(const jaeger_host_port* host_port,
                              int socket_type,
                              struct addrinfo** host_addrs);

#ifdef __cplusplus
} /* extern C */
#endif /* __cplusplus */

#endif /* JAEGERTRACINGC_NET_H */
