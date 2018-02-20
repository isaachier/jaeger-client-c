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

#include "jaegertracingc/reporter.h"

#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "jaegertracingc/span.h"
#include "jaegertracingc/threading.h"
#include "jaegertracingc/tracer.h"
#include "unity.h"

#define MAX_PORT_LEN 5

static inline int start_udp_server()
{
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    const int fd = socket(AF_INET, SOCK_DGRAM, 0);
    TEST_ASSERT_GREATER_OR_EQUAL(0, fd);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    TEST_ASSERT_EQUAL(0, bind(fd, (struct sockaddr*) &addr, sizeof(addr)));
    return fd;
}

static void* flush_reporter(void* arg)
{
    assert(arg != NULL);
    const int num_flushed =
        jaeger_remote_reporter_flush(arg, jaeger_null_logger());
    int* return_value = jaeger_malloc(sizeof(int));
    TEST_ASSERT_NOT_NULL(return_value);
    *return_value = num_flushed;
    return return_value;
}

void test_reporter()
{
    jaeger_logger* logger = jaeger_null_logger();
    jaeger_span span;
    TEST_ASSERT_TRUE(jaeger_span_init(&span, logger));
    /* TODO: Update once tracer is implemented */
    jaeger_tracer tracer;
    tracer.service_name = "test service";
    TEST_ASSERT_TRUE(
        jaeger_vector_init(&tracer.tags, sizeof(jaeger_tag), NULL, logger));
    span.tracer = &tracer;
    span.operation_name = jaeger_strdup("test-operation", logger);
    TEST_ASSERT_NOT_NULL(span.operation_name);
    jaeger_log_record* log_ptr = jaeger_vector_append(&span.logs, logger);
    TEST_ASSERT_NOT_NULL(log_ptr);
    TEST_ASSERT_TRUE(jaeger_log_record_init(log_ptr, logger));

    jaeger_reporter* r = jaeger_null_reporter();
    r->report(r, &span, logger);
    r->destroy((jaeger_destructible*) r);

    jaeger_logging_reporter_init(r);
    r->report(r, &span, logger);
    r->destroy((jaeger_destructible*) r);

    jaeger_in_memory_reporter in_memory_reporter;
    TEST_ASSERT_TRUE(
        jaeger_in_memory_reporter_init(&in_memory_reporter, logger));
    r = (jaeger_reporter*) &in_memory_reporter;
    r->report(r, &span, logger);
    r->destroy((jaeger_destructible*) r);

    jaeger_composite_reporter composite_reporter;
    TEST_ASSERT_TRUE(
        jaeger_composite_reporter_init(&composite_reporter, logger));
    r = (jaeger_reporter*) &composite_reporter;
    r->report(r, &span, logger);
    r->destroy((jaeger_destructible*) r);

    const int server_fd = start_udp_server();
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    socklen_t addr_len = sizeof(addr);
    TEST_ASSERT_EQUAL(
        0, getsockname(server_fd, (struct sockaddr*) &addr, &addr_len));
    TEST_ASSERT_EQUAL(sizeof(addr), addr_len);
    char host_port[INET_ADDRSTRLEN + 1 + MAX_PORT_LEN];
    TEST_ASSERT_NOT_NULL(
        inet_ntop(AF_INET, &addr.sin_addr, host_port, sizeof(host_port)));
    const int host_end = strlen(host_port);
    TEST_ASSERT_LESS_THAN(sizeof(host_port) - host_end,
                          snprintf(&host_port[host_end],
                                   sizeof(host_port) - host_end,
                                   ":%d",
                                   ntohs(addr.sin_port)));

    jaeger_remote_reporter remote_reporter;

    jaeger_metrics* metrics = jaeger_null_metrics();
    TEST_ASSERT_TRUE(jaeger_remote_reporter_init(
        &remote_reporter, host_port, 0, metrics, logger));
    r = (jaeger_reporter*) &remote_reporter;
    r->report(r, &span, logger);
    jaeger_thread thread;
    TEST_ASSERT_EQUAL(
        0, jaeger_thread_init(&thread, &flush_reporter, &remote_reporter));
    char buffer[1024];
    const int num_read = recv(server_fd, buffer, sizeof(buffer), 0);
    Jaegertracing__Protobuf__Batch* batch =
        jaegertracing__protobuf__batch__unpack(
            NULL, num_read, (const uint8_t*) buffer);
    TEST_ASSERT_NOT_NULL(batch);
    void* num_flushed = NULL;
    jaeger_thread_join(thread, &num_flushed);
    TEST_ASSERT_EQUAL(*(int*) num_flushed, batch->n_spans);
    jaegertracing__protobuf__batch__free_unpacked(batch, NULL);
    r->destroy((jaeger_destructible*) r);
    jaeger_free(num_flushed);

    const int small_packet_size = 1;
    TEST_ASSERT_TRUE(jaeger_remote_reporter_init(
        &remote_reporter, host_port, small_packet_size, metrics, logger));
    r->report(r, &span, logger);
    TEST_ASSERT_EQUAL(
        0, jaeger_thread_init(&thread, &flush_reporter, &remote_reporter));
    num_flushed = NULL;
    jaeger_thread_join(thread, &num_flushed);
    TEST_ASSERT_EQUAL(-1, *(int*) num_flushed);
    jaeger_free(num_flushed);
    r->destroy((jaeger_destructible*) r);

    close(server_fd);

    jaeger_span_destroy(&span);

    /* TODO: Update once tracer is implemented */
    JAEGERTRACINGC_VECTOR_FOR_EACH(&tracer.tags, jaeger_tag_destroy);
    jaeger_vector_destroy(&tracer.tags);
}
