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

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include <evhtp/evhtp.h>

static void health_check(evhtp_request_t* req, void* arg)
{
    (void) arg;
    switch (strlen(req->uri->path->full)) {
    case 0:
        break;
    case 1:
        if (req->uri->path->full[0] == '/') {
            break;
        }
        /* FALLTHRU */
    default:
        evhtp_send_reply(req, EVHTP_RES_NOTFOUND);
        return;
    }
    evhtp_send_reply(req, EVHTP_RES_OK);
}

static void start_trace(evhtp_request_t* req, void* arg)
{
    /* TODO */
    (void) req;
    (void) arg;
    evhtp_send_reply(req, EVHTP_RES_OK);
}

static void join_trace(evhtp_request_t* req, void* arg)
{
    /* TODO */
    (void) req;
    (void) arg;
    evhtp_send_reply(req, EVHTP_RES_OK);
}

static void create_traces(evhtp_request_t* req, void* arg)
{
    /* TODO */
    (void) req;
    (void) arg;
    evhtp_send_reply(req, EVHTP_RES_OK);
}

int main()
{
    struct event_base* evbase = event_base_new();
    evhtp_t* htp = evhtp_new(evbase, NULL);
    assert(evbase != NULL);
    assert(htp != NULL);

    evhtp_set_gencb(htp, &health_check, NULL);
    evhtp_set_cb(htp, "/start_trace", &start_trace, NULL);
    evhtp_set_cb(htp, "/join_trace", &join_trace, NULL);
    evhtp_set_cb(htp, "/create_traces", &create_traces, NULL);
    evhtp_enable_flag(htp, EVHTP_FLAG_ENABLE_ALL);

    struct sockaddr_in addr;
    enum { port = 8080, backlog = 128 };
    evhtp_bind_socket(htp, "127.0.0.1", port, backlog);

    event_base_loop(evbase, 0);
    return 0;
}
