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
    jaeger_span span = JAEGERTRACINGC_SPAN_INIT;
    TEST_ASSERT_TRUE(jaeger_span_init(&span, logger));
    span.context.flags = jaeger_sampling_flag_sampled;

    /* TODO: Update once tracer is implemented */
    jaeger_tracer tracer;
    tracer.service_name = "test service";
    TEST_ASSERT_TRUE(
        jaeger_vector_init(&tracer.tags, sizeof(jaeger_tag), NULL, logger));
    span.tracer = &tracer;
    TEST_ASSERT_NULL(span.operation_name);
    TEST_ASSERT_TRUE(
        jaeger_span_set_operation_name(&span, "test-operation", logger));
    TEST_ASSERT_NOT_NULL(span.operation_name);

    jaeger_log_record log = JAEGERTRACINGC_LOG_RECORD_INIT;
    TEST_ASSERT_TRUE(jaeger_log_record_init(&log, logger));
    jaeger_tag* field = jaeger_vector_append(&log.fields, logger);
    TEST_ASSERT_NOT_NULL(field);
    *field = (jaeger_tag) JAEGERTRACINGC_TAG_INIT;
    TEST_ASSERT_TRUE(jaeger_tag_init(field, "key", logger));
    field->value_case = JAEGERTRACINGC_TAG_TYPE(STR);
    field->str_value = jaeger_strdup("value", logger);
    TEST_ASSERT_NOT_NULL(field->str_value);

    TEST_ASSERT_TRUE(jaeger_span_log(&span, &log, logger));
    jaeger_log_record_destroy(&log);

    jaeger_span_ref* span_ref_ptr = jaeger_vector_append(&span.refs, logger);
    TEST_ASSERT_NOT_NULL(span_ref_ptr);
    span_ref_ptr->context =
        (jaeger_span_context){.trace_id = {.high = 0xDEAD, .low = 0xBEEF},
                              .span_id = 0xCAFE,
                              .parent_id = 0,
                              .flags = 0};
    span_ref_ptr->type = JAEGERTRACING__PROTOBUF__SPAN_REF__TYPE__CHILD_OF;

    jaeger_key_value* kv = jaeger_vector_append(&span.context.baggage, logger);
    TEST_ASSERT_NOT_NULL(kv);
    *kv = (jaeger_key_value) JAEGERTRACINGC_KEY_VALUE_INIT;
    TEST_ASSERT_TRUE(jaeger_key_value_init(kv, "key", "value", logger));

    jaeger_span_finish_with_options(&span, NULL);

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

    /* Test connecting to a server address, closing the server socket, then
     * attempting a flush.
     */
    int fake_socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in fake_server_addr;
    memset(&fake_server_addr, 0, sizeof(fake_server_addr));
    TEST_ASSERT_EQUAL(0,
                      bind(fake_socket_fd,
                           (struct sockaddr*) &fake_server_addr,
                           sizeof(fake_server_addr)));
    socklen_t fake_server_addr_len = sizeof(fake_server_addr);
    TEST_ASSERT_EQUAL(0,
                      getsockname(fake_socket_fd,
                                  (struct sockaddr*) &fake_server_addr,
                                  &fake_server_addr_len));
    TEST_ASSERT_EQUAL(sizeof(fake_server_addr), fake_server_addr_len);
    char fake_host_port[INET_ADDRSTRLEN + 1 + MAX_PORT_LEN];
    TEST_ASSERT_NOT_NULL(inet_ntop(AF_INET,
                                   &fake_server_addr.sin_addr,
                                   fake_host_port,
                                   sizeof(fake_host_port)));
    const int fake_host_end = strlen(fake_host_port);
    TEST_ASSERT_LESS_THAN(sizeof(fake_host_port) - fake_host_end,
                          snprintf(&fake_host_port[fake_host_end],
                                   sizeof(fake_host_port) - fake_host_end,
                                   ":%d",
                                   ntohs(fake_server_addr.sin_port)));
    jaeger_remote_reporter remote_reporter;
    char buffer[1024];
    jaeger_metrics* metrics = jaeger_null_metrics();
    TEST_ASSERT_TRUE(jaeger_remote_reporter_init(
        &remote_reporter, fake_host_port, sizeof(buffer), metrics, logger));
    r->report(r, &span, logger);
    close(fake_socket_fd);
    TEST_ASSERT_FALSE(jaeger_remote_reporter_flush(&remote_reporter, logger));
    remote_reporter.destroy((jaeger_destructible*) &remote_reporter);

    /* Test connection to real server works as expected. */
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

    TEST_ASSERT_TRUE(jaeger_remote_reporter_init(
        &remote_reporter, host_port, sizeof(buffer), metrics, logger));
    r = (jaeger_reporter*) &remote_reporter;
    for (int i = 0; i < 100; i++) {
        r->report(r, &span, logger);
    }
    jaeger_thread thread;
    TEST_ASSERT_EQUAL(
        0, jaeger_thread_init(&thread, &flush_reporter, &remote_reporter));
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
    JAEGERTRACINGC_VECTOR_FOR_EACH(
        &tracer.tags, jaeger_tag_destroy, jaeger_tag);
    jaeger_vector_destroy(&tracer.tags);
}
