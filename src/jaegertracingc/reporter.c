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
#include "jaegertracingc/threading.h"
#include "jaegertracingc/tracer.h"

static jaeger_reporter null_reporter;

static void null_destroy(jaeger_destructible* destructible)
{
    (void) destructible;
}

static void null_report(jaeger_reporter* reporter,
                        const jaeger_span* span,
                        jaeger_logger* logger)
{
    (void) reporter;
    (void) span;
}

static void init_null_reporter()
{
    null_reporter =
        (jaeger_reporter){.destroy = &null_destroy, .report = &null_report};
}

jaeger_reporter* jaeger_null_reporter()
{
    static jaeger_once once = JAEGERTRACINGC_ONCE_INIT;
    jaeger_do_once(&once, &init_null_reporter);
    return &null_reporter;
}

static void logging_reporter_report(jaeger_reporter* reporter,
                                    const jaeger_span* span,
                                    jaeger_logger* logger)
{
    assert(reporter != NULL);
    if (logger != NULL && span != NULL) {
        char buffer[JAEGERTRACINGC_SPAN_CONTEXT_MAX_STR_LEN + 1];
        buffer[JAEGERTRACINGC_SPAN_CONTEXT_MAX_STR_LEN] = '\0';
        logger->info(logger, "%s", buffer);
    }
}

void jaeger_logging_reporter_init(jaeger_reporter* reporter)
{
    assert(reporter != NULL);
    reporter->destroy = &null_destroy;
    reporter->report = &logging_reporter_report;
}

static void in_memory_reporter_destroy(jaeger_destructible* destructible)
{
    if (destructible == NULL) {
        return;
    }

    jaeger_in_memory_reporter* r = (jaeger_in_memory_reporter*) destructible;
    JAEGERTRACINGC_VECTOR_FOR_EACH(&r->spans, jaeger_span_destroy);
    jaeger_vector_destroy(&r->spans);
    jaeger_mutex_destroy(&r->mutex);
}

static void in_memory_reporter_report(jaeger_reporter* reporter,
                                      const jaeger_span* span,
                                      jaeger_logger* logger)
{
    assert(reporter != NULL);
    jaeger_in_memory_reporter* r = (jaeger_in_memory_reporter*) reporter;
    if (span == NULL) {
        return;
    }
    jaeger_mutex_lock(&r->mutex);
    jaeger_span* span_copy = jaeger_vector_append(&r->spans, NULL);
    if (span_copy != NULL) {
        if (jaeger_span_init(span_copy, logger)) {
            jaeger_span_copy(span_copy, span, logger);
        }
        else {
            jaeger_free(span_copy);
        }
    }
    jaeger_mutex_unlock(&r->mutex);
}

bool jaeger_in_memory_reporter_init(jaeger_in_memory_reporter* reporter,
                                    jaeger_logger* logger)
{
    assert(reporter != NULL);
    if (!jaeger_vector_init(
            &reporter->spans, sizeof(jaeger_span), NULL, logger)) {
        return false;
    }
    reporter->destroy = &in_memory_reporter_destroy;
    reporter->report = &in_memory_reporter_report;
    reporter->mutex = (jaeger_mutex) JAEGERTRACINGC_MUTEX_INIT;
    return true;
}

static void composite_reporter_destroy(jaeger_destructible* destructible)
{
    if (destructible == NULL) {
        return;
    }
    jaeger_composite_reporter* r = (jaeger_composite_reporter*) destructible;

#define DESTROY(x)                   \
    do {                             \
        assert(x != NULL);           \
        jaeger_destructible** d = x; \
        if (*d == NULL) {            \
            continue;                \
        }                            \
        (*d)->destroy(*d);           \
        jaeger_free(*d);             \
    } while (0)

    JAEGERTRACINGC_VECTOR_FOR_EACH(&r->reporters, DESTROY);

#undef DESTROY

    jaeger_vector_destroy(&r->reporters);
}

static void composite_reporter_report(jaeger_reporter* reporter,
                                      const jaeger_span* span,
                                      jaeger_logger* logger)
{
    assert(reporter != NULL);
    if (span != NULL) {
        jaeger_composite_reporter* r = (jaeger_composite_reporter*) reporter;

#define REPORT(x)                                                 \
    do {                                                          \
        if (x == NULL) {                                          \
            continue;                                             \
        }                                                         \
        jaeger_reporter** child_reporter = x;                     \
        if (*child_reporter == NULL) {                            \
            continue;                                             \
        }                                                         \
        (*child_reporter)->report(*child_reporter, span, logger); \
    } while (0)

        JAEGERTRACINGC_VECTOR_FOR_EACH(&r->reporters, REPORT);

#undef REPORT
    }
}

bool jaeger_composite_reporter_init(jaeger_composite_reporter* reporter,
                                    jaeger_logger* logger)
{
    assert(reporter != NULL);
    if (!jaeger_vector_init(
            &reporter->reporters, sizeof(jaeger_reporter*), NULL, logger)) {
        return false;
    }

    reporter->destroy = &composite_reporter_destroy;
    reporter->report = &composite_reporter_report;
    return true;
}

static inline void process_destroy(Jaegertracing__Protobuf__Process* process)
{
    if (process == NULL) {
        return;
    }
    if (process->tags != NULL) {
        for (int i = 0; i < process->n_tags; i++) {
            if (process->tags[i] != NULL) {
                jaeger_tag_destroy(process->tags[i]);
                jaeger_free(process->tags[i]);
            }
        }
        jaeger_free(process->tags);
        process->tags = NULL;
        process->n_tags = 0;
    }
    if (process->service_name != NULL && strlen(process->service_name) > 0) {
        jaeger_free(process->service_name);
        process->service_name = NULL;
    }
}

static inline bool build_process(Jaegertracing__Protobuf__Process* process,
                                 const jaeger_tracer* tracer,
                                 jaeger_logger* logger)
{
    assert(process != NULL);
    assert(tracer != NULL);
    if (tracer->service_name == NULL || strlen(tracer->service_name) == 0) {
        if (logger != NULL) {
            logger->error(logger, "Invalid null or empty service name");
        }
        return false;
    }
    process->service_name = jaeger_strdup(tracer->service_name, logger);
    if (process->service_name == NULL) {
        goto cleanup;
    }

    if (!jaeger_vector_protobuf_copy((void***) &process->tags,
                                     &process->n_tags,
                                     &tracer->tags,
                                     sizeof(jaeger_tag),
                                     &jaeger_tag_copy_wrapper,
                                     &jaeger_tag_destroy_wrapper,
                                     NULL,
                                     logger)) {
        goto cleanup;
    }
    return true;

cleanup:
    process_destroy(process);
    jaeger_free(process);
    return false;
}

static inline void large_batch_error(Jaegertracing__Protobuf__Batch* batch,
                                     jaeger_vector* spans,
                                     int initial_num_spans,
                                     int max_packet_size,
                                     jaeger_metrics* metrics,
                                     jaeger_logger* logger)
{
    if (logger != NULL) {
        Jaegertracing__Protobuf__Batch tmp = *batch;
        memcpy(&tmp, batch, sizeof(tmp));
        tmp.n_spans = 1;
        logger->error(logger,
                      "Message is too large to send in a single packet, "
                      "minimum message size = %d, "
                      "maximum packet size = %d",
                      jaegertracing__protobuf__batch__get_packed_size(&tmp),
                      max_packet_size);
    }

    const int batch_packed_size =
        jaegertracing__protobuf__batch__get_packed_size(batch);
    if (batch_packed_size > max_packet_size && logger != NULL) {
        logger->error(logger,
                      "Detected batch with zero spans exceeds maximum "
                      "packet size, "
                      "batch size (zero spans) = %d, "
                      "maximum packet size = %d",
                      batch_packed_size,
                      max_packet_size);
    }

    if (logger != NULL) {
        logger->error(logger, "Dropping first span to avoid repeated failures");
    }

    spans->data = batch->spans;
    spans->len = initial_num_spans;
    *batch =
        (Jaegertracing__Protobuf__Batch) JAEGERTRACING__PROTOBUF__BATCH__INIT;
    Jaegertracing__Protobuf__Span** span = jaeger_vector_get(spans, 0, logger);
    jaeger_span_protobuf_destroy(*span);
    jaeger_free(*span);
    jaeger_vector_remove(spans, 0, logger);

    if (metrics != NULL) {
        jaeger_gauge* queue_length = metrics->reporter_queue_length;
        assert(queue_length != NULL);
        queue_length->update(queue_length, jaeger_vector_length(spans));

        jaeger_counter* dropped = metrics->reporter_dropped;
        assert(dropped != NULL);
        dropped->inc(dropped, 1);
    }
}

static inline bool build_batch(Jaegertracing__Protobuf__Batch* batch,
                               Jaegertracing__Protobuf__Process* process,
                               jaeger_vector* spans,
                               int max_packet_size,
                               jaeger_metrics* metrics,
                               jaeger_logger* logger)
{
    assert(batch != NULL);
    assert(process != NULL);
    assert(spans != NULL);
    batch->process = process;

    const int num_spans = jaeger_vector_length(spans);
    batch->n_spans = num_spans;
    batch->spans = spans->data;
    spans->len = 0;
    spans->data = NULL;

    for (; batch->n_spans > 0 &&
           jaegertracing__protobuf__batch__get_packed_size(batch) >
               max_packet_size;
         batch->n_spans--)
        ;
    if (batch->n_spans == 0) {
        large_batch_error(
            batch, spans, num_spans, max_packet_size, metrics, logger);
        return false;
    }

    const int remaining_spans = num_spans - batch->n_spans;
    if (remaining_spans > 0 &&
        jaeger_vector_init(
            spans, sizeof(Jaegertracing__Protobuf__Span*), NULL, logger) &&
        jaeger_vector_reserve(spans, remaining_spans, logger)) {
        for (int i = 0; i < remaining_spans; i++) {
            Jaegertracing__Protobuf__Span** span =
                jaeger_vector_append(spans, logger);
            assert(span != NULL);
            *span = batch->spans[batch->n_spans + i];
        }
    }
    else if (remaining_spans > 0) {
        if (logger != NULL) {
            logger->error(logger,
                          "Cannot allocate space for span buffer, must "
                          "drop remaining spans, num dropped spans = %d",
                          remaining_spans);
        }
        for (int i = 0; i < remaining_spans; i++) {
            jaeger_span_protobuf_destroy(batch->spans[batch->n_spans + i]);
            jaeger_free(batch->spans[batch->n_spans + i]);
        }
        if (metrics != NULL) {
            jaeger_counter* dropped = metrics->reporter_dropped;
            assert(dropped != NULL);
            dropped->inc(dropped, 1);
        }
    }
    return true;
}

static void remote_reporter_destroy(jaeger_destructible* destructible)
{
    if (destructible == NULL) {
        return;
    }
    jaeger_remote_reporter* r = (jaeger_remote_reporter*) destructible;

    if (r->fd >= 0) {
        close(r->fd);
        r->fd = -1;
    }

    process_destroy(&r->process);

    for (int i = 0, len = jaeger_vector_length(&r->spans); i < len; i++) {
        Jaegertracing__Protobuf__Span** span =
            jaeger_vector_offset(&r->spans, i);
        assert(span != NULL);
        if (*span != NULL) {
            continue;
        }
        jaeger_span_protobuf_destroy(*span);
        jaeger_free(*span);
    }
    jaeger_vector_destroy(&r->spans);
}

static void remote_reporter_report(jaeger_reporter* reporter,
                                   const jaeger_span* span,
                                   jaeger_logger* logger)
{
    assert(reporter != NULL);
    if (span == NULL) {
        return;
    }

    jaeger_remote_reporter* r = (jaeger_remote_reporter*) reporter;
    if (!jaeger_vector_reserve(
            &r->spans, jaeger_vector_length(&r->spans) + 1, logger)) {
        goto update_counter;
    }

    Jaegertracing__Protobuf__Span* span_copy =
        jaeger_malloc(sizeof(Jaegertracing__Protobuf__Span));
    if (span_copy == NULL) {
        logger->error(logger, "Cannot allocate span for reporter batch");
        goto update_counter;
    }
    *span_copy =
        (Jaegertracing__Protobuf__Span) JAEGERTRACING__PROTOBUF__SPAN__INIT;
    if (!jaeger_span_to_protobuf(span_copy, span, logger)) {
        goto cleanup;
    }
    Jaegertracing__Protobuf__Span** span_ptr =
        jaeger_vector_append(&r->spans, logger);
    assert(span_ptr != NULL);
    *span_ptr = span_copy;

    if (r->metrics != NULL) {
        jaeger_gauge* queue_length = r->metrics->reporter_queue_length;
        assert(queue_length != NULL);
        queue_length->update(queue_length, jaeger_vector_length(&r->spans));
    }

    if (r->process.service_name == NULL) {
        jaeger_mutex_lock((jaeger_mutex*) &span->mutex);
        /* Building process will not affect this span, so ignore return value.
         */
        build_process(&r->process, span->tracer, logger);
        jaeger_mutex_unlock((jaeger_mutex*) &span->mutex);
    }

    return;

cleanup:
    jaeger_span_protobuf_destroy(span_copy);
    jaeger_free(span_copy);
update_counter:
    if (r->metrics != NULL) {
        jaeger_counter* dropped = r->metrics->reporter_dropped;
        assert(dropped != NULL);
        dropped->inc(dropped, 1);
    }
}

bool jaeger_remote_reporter_init(jaeger_remote_reporter* reporter,
                                 const char* host_port_str,
                                 int max_packet_size,
                                 jaeger_metrics* metrics,
                                 jaeger_logger* logger)
{
    assert(reporter != NULL);

    const int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) {
        return false;
    }
    reporter->fd = fd;

    reporter->process = (Jaegertracing__Protobuf__Process)
        JAEGERTRACING__PROTOBUF__PROCESS__INIT;
    reporter->max_packet_size = (max_packet_size > 0)
                                    ? max_packet_size
                                    : JAEGERTRACINGC_DEFAULT_UDP_BUFFER_SIZE;
    if (!jaeger_vector_init(&reporter->spans,
                            sizeof(Jaegertracing__Protobuf__Span*),
                            NULL,
                            logger)) {
        goto cleanup;
    }

    jaeger_host_port host_port =
        (jaeger_host_port) JAEGERTRACINGC_HOST_PORT_INIT;
    if (host_port_str == NULL || strlen(host_port_str) == 0) {
        host_port_str = JAEGERTRACINGC_DEFAULT_UDP_SPAN_SERVER_HOST_PORT;
    }
    if (!jaeger_host_port_scan(&host_port, host_port_str, logger)) {
        goto cleanup;
    }
    struct addrinfo* addrs;
    if (!jaeger_host_port_resolve(&host_port, SOCK_DGRAM, &addrs, logger)) {
        goto cleanup;
    }
    bool success = false;
    for (struct addrinfo* iter = addrs; iter != NULL; iter = iter->ai_next) {
        if (connect(fd, iter->ai_addr, iter->ai_addrlen) == 0 &&
            sizeof(reporter->addr) == iter->ai_addrlen) {
            memcpy(&reporter->addr, iter->ai_addr, sizeof(reporter->addr));
            success = true;
            break;
        }
    }
    freeaddrinfo(addrs);
    if (!success) {
        if (logger != NULL) {
            logger->error(logger,
                          "Failed to resolve remote reporter host port, host "
                          "port = \"%s\"",
                          host_port_str);
        }
        goto cleanup;
    }
    jaeger_host_port_destroy(&host_port);
    reporter->metrics = metrics;
    reporter->destroy = &remote_reporter_destroy;
    reporter->report = &remote_reporter_report;
    return true;

cleanup:
    jaeger_host_port_destroy(&host_port);
    remote_reporter_destroy((jaeger_destructible*) reporter);
    return false;
}

int jaeger_remote_reporter_flush(jaeger_remote_reporter* reporter,
                                 jaeger_logger* logger)
{
    assert(reporter != NULL);

    const int num_spans = jaeger_vector_length(&reporter->spans);
    if (num_spans == 0) {
        return 0;
    }

    Jaegertracing__Protobuf__Batch batch = JAEGERTRACING__PROTOBUF__BATCH__INIT;
    if (!build_batch(&batch,
                     &reporter->process,
                     &reporter->spans,
                     reporter->max_packet_size,
                     reporter->metrics,
                     logger)) {
        return -1;
    }

    uint8_t buffer[reporter->max_packet_size];
    ProtobufCBufferSimple simple = PROTOBUF_C_BUFFER_SIMPLE_INIT(buffer);
    jaegertracing__protobuf__batch__pack_to_buffer(&batch,
                                                   (ProtobufCBuffer*) &simple);
    assert(simple.len <= reporter->max_packet_size);
    const int num_written = sendto(reporter->fd,
                                   simple.data,
                                   simple.len,
                                   0,
                                   (struct sockaddr*) &reporter->addr,
                                   sizeof(reporter->addr));
    if (num_written != simple.len) {
        if (logger != NULL) {
            logger->error(logger,
                          "Cannot write entire message to UDP socket, "
                          "num written = %d",
                          num_written);
        }
        goto cleanup;
    }
    PROTOBUF_C_BUFFER_SIMPLE_CLEAR(&simple);
    const int num_flushed = batch.n_spans;
    for (int i = 0; i < num_flushed; i++) {
        if (batch.spans[i] != NULL) {
            jaeger_span_protobuf_destroy(batch.spans[i]);
            jaeger_free(batch.spans[i]);
        }
    }
    jaeger_free(batch.spans);
    if (reporter->metrics != NULL) {
        jaeger_counter* num_success = reporter->metrics->reporter_success;
        assert(num_success != NULL);
        num_success->inc(num_success, num_flushed);
    }
    return num_flushed;

cleanup:
    if (reporter->spans.data != NULL) {
        /* No need to delete the span pointers because they are still valid and
         * in batch's buffer. */
        jaeger_free(reporter->spans.data);
        reporter->spans.data = NULL;
    }
    reporter->spans.data = batch.spans;
    reporter->spans.len = num_spans;
    return -1;
}
