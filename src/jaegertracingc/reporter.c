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

#include <errno.h>

#include "jaegertracingc/threading.h"
#include "jaegertracingc/tracer.h"

static jaeger_reporter null_reporter;

static void null_destroy(jaeger_destructible* destructible)
{
    (void) destructible;
}

static void null_report(jaeger_reporter* reporter, const jaeger_span* span)
{
    (void) reporter;
    (void) span;
}

static bool null_flush(jaeger_reporter* reporter)
{
    (void) reporter;
    return true;
}

static void init_null_reporter()
{
    null_reporter = (jaeger_reporter){.base = {.destroy = &null_destroy},
                                      .report = &null_report,
                                      .flush = &null_flush};
}

jaeger_reporter* jaeger_null_reporter()
{
    static jaeger_once once = JAEGERTRACINGC_ONCE_INIT;
    jaeger_do_once(&once, &init_null_reporter);
    return &null_reporter;
}

static void logging_reporter_report(jaeger_reporter* reporter,
                                    const jaeger_span* span)
{
    (void) reporter;
    if (span != NULL) {
        char buffer[JAEGERTRACINGC_SPAN_CONTEXT_MAX_STR_LEN + 1];
        buffer[JAEGERTRACINGC_SPAN_CONTEXT_MAX_STR_LEN] = '\0';
        jaeger_log_info("%s", buffer);
    }
}

void jaeger_logging_reporter_init(jaeger_reporter* reporter)
{
    assert(reporter != NULL);
    ((jaeger_destructible*) reporter)->destroy = &null_destroy;
    reporter->report = &logging_reporter_report;
    reporter->flush = &null_flush;
}

static void in_memory_reporter_destroy(jaeger_destructible* destructible)
{
    assert(destructible != NULL);
    jaeger_in_memory_reporter* r = (jaeger_in_memory_reporter*) destructible;

    JAEGERTRACINGC_VECTOR_FOR_EACH(
        &r->spans, jaeger_span_destroy, jaeger_destructible);

    jaeger_vector_destroy(&r->spans);
    jaeger_mutex_destroy(&r->mutex);
}

static void in_memory_reporter_report(jaeger_reporter* reporter,
                                      const jaeger_span* span)
{
    assert(reporter != NULL);
    if (span == NULL) {
        return;
    }
    jaeger_in_memory_reporter* r = (jaeger_in_memory_reporter*) reporter;
    jaeger_mutex_lock(&r->mutex);
    jaeger_span* span_copy = jaeger_vector_append(&r->spans);
    if (span_copy != NULL && !jaeger_span_copy(span_copy, span)) {
        r->spans.len--;
    }
    jaeger_mutex_unlock(&r->mutex);
}

bool jaeger_in_memory_reporter_init(jaeger_in_memory_reporter* reporter)
{
    assert(reporter != NULL);
    if (!jaeger_vector_init(&reporter->spans, sizeof(jaeger_span))) {
        return false;
    }
    ((jaeger_destructible*) reporter)->destroy = &in_memory_reporter_destroy;
    ((jaeger_reporter*) reporter)->report = &in_memory_reporter_report;
    ((jaeger_reporter*) reporter)->flush = &null_flush;
    reporter->mutex = (jaeger_mutex) JAEGERTRACINGC_MUTEX_INIT;
    return true;
}

static void composite_reporter_destroy(jaeger_destructible* destructible)
{
    assert(destructible != NULL);
    jaeger_composite_reporter* r = (jaeger_composite_reporter*) destructible;

#define DESTROY(x)                     \
    do {                               \
        jaeger_destructible** d = (x); \
        if (d != NULL && *d != NULL) { \
            (*d)->destroy(*d);         \
            jaeger_free(*d);           \
        }                              \
    } while (0)

    JAEGERTRACINGC_VECTOR_FOR_EACH(
        &r->reporters, DESTROY, jaeger_destructible*);

#undef DESTROY

    jaeger_vector_destroy(&r->reporters);
}

static void composite_reporter_report(jaeger_reporter* reporter,
                                      const jaeger_span* span)
{
    assert(reporter != NULL);
    if (span == NULL) {
        return;
    }
    jaeger_composite_reporter* r = (jaeger_composite_reporter*) reporter;

#define REPORT(x)                                                \
    do {                                                         \
        jaeger_reporter** child_reporter = (x);                  \
        if (child_reporter != NULL && *child_reporter != NULL) { \
            (*child_reporter)->report(*child_reporter, span);    \
        }                                                        \
    } while (0)

    JAEGERTRACINGC_VECTOR_FOR_EACH(&r->reporters, REPORT, jaeger_reporter*);

#undef REPORT
}

static bool composite_reporter_flush(jaeger_reporter* reporter)
{
    jaeger_composite_reporter* r = (jaeger_composite_reporter*) reporter;

#define FLUSH(x)                                                 \
    do {                                                         \
        jaeger_reporter** child_reporter = (x);                  \
        if (child_reporter != NULL && *child_reporter != NULL && \
            !(*child_reporter)->flush(*child_reporter)) {        \
            success = false;                                     \
        }                                                        \
    } while (0)

    bool success = true;
    JAEGERTRACINGC_VECTOR_FOR_EACH(&r->reporters, FLUSH, jaeger_reporter*);
    return success;

#undef FLUSH
}

bool jaeger_composite_reporter_init(jaeger_composite_reporter* reporter)
{
    assert(reporter != NULL);
    if (!jaeger_vector_init(&reporter->reporters, sizeof(jaeger_reporter*))) {
        return false;
    }

    ((jaeger_destructible*) reporter)->destroy = &composite_reporter_destroy;
    ((jaeger_reporter*) reporter)->report = &composite_reporter_report;
    ((jaeger_reporter*) reporter)->flush = &composite_reporter_flush;
    return true;
}

static inline void process_destroy(Jaegertracing__Protobuf__Process* process)
{
    assert(process != NULL);
    if (process->tags != NULL) {
        for (int i = 0; i < (int) process->n_tags; i++) {
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
                                 const jaeger_tracer* tracer)
{
    assert(process != NULL);
    assert(tracer != NULL);
    assert(tracer->service_name != NULL);
    assert(strlen(tracer->service_name) > 0);
    process->service_name = jaeger_strdup(tracer->service_name);
    if (process->service_name == NULL) {
        goto cleanup;
    }

    if (!jaeger_vector_protobuf_copy((void***) &process->tags,
                                     &process->n_tags,
                                     &tracer->tags,
                                     sizeof(jaeger_tag),
                                     &jaeger_tag_copy_wrapper,
                                     &jaeger_tag_destroy_wrapper,
                                     NULL)) {
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
                                     jaeger_metrics* metrics)
{
    Jaegertracing__Protobuf__Batch tmp = *batch;
    memcpy(&tmp, batch, sizeof(tmp));
    tmp.n_spans = 1;
    jaeger_log_error("Message is too large to send in a single packet, "
                     "minimum message size = %zu, "
                     "maximum packet size = %d",
                     jaegertracing__protobuf__batch__get_packed_size(&tmp),
                     max_packet_size);

    const int batch_packed_size =
        jaegertracing__protobuf__batch__get_packed_size(batch);
    if (batch_packed_size > max_packet_size) {
        jaeger_log_error("Detected batch with zero spans exceeds maximum "
                         "packet size, "
                         "batch size (zero spans) = %d, "
                         "maximum packet size = %d",
                         batch_packed_size,
                         max_packet_size);
    }

    jaeger_log_error("Dropping first span to avoid repeated failures");

    spans->data = (char*) batch->spans;
    spans->len = initial_num_spans;
    *batch =
        (Jaegertracing__Protobuf__Batch) JAEGERTRACING__PROTOBUF__BATCH__INIT;
    Jaegertracing__Protobuf__Span** span = jaeger_vector_get(spans, 0);
    jaeger_span_protobuf_destroy(*span);
    jaeger_free(*span);
    jaeger_vector_remove(spans, 0);

    if (metrics != NULL) {
        jaeger_counter* dropped = metrics->reporter_dropped;
        assert(dropped != NULL);
        dropped->inc(dropped, 1);
    }
}

static inline bool build_batch(Jaegertracing__Protobuf__Batch* batch,
                               Jaegertracing__Protobuf__Process* process,
                               jaeger_vector* spans,
                               int max_packet_size,
                               jaeger_metrics* metrics)
{
    assert(batch != NULL);
    assert(process != NULL);
    assert(spans != NULL);
    batch->process = process;

    const int num_spans = jaeger_vector_length(spans);
    batch->n_spans = num_spans;
    batch->spans = (Jaegertracing__Protobuf__Span**) spans->data;
    spans->len = 0;
    spans->data = NULL;

    for (; batch->n_spans > 0 &&
           (int) jaegertracing__protobuf__batch__get_packed_size(batch) >
               max_packet_size;
         batch->n_spans--) {
    }
    if (batch->n_spans == 0) {
        large_batch_error(batch, spans, num_spans, max_packet_size, metrics);
        return false;
    }

    const int remaining_spans = num_spans - batch->n_spans;
    if (remaining_spans > 0 &&
        jaeger_vector_init(spans, sizeof(Jaegertracing__Protobuf__Span*)) &&
        jaeger_vector_reserve(spans, remaining_spans)) {
        for (int i = 0; i < remaining_spans; i++) {
            Jaegertracing__Protobuf__Span** span = jaeger_vector_append(spans);
            assert(span != NULL);
            *span = batch->spans[batch->n_spans + i];
        }
    }
    else if (remaining_spans > 0) {
        jaeger_log_error("Cannot allocate space for span buffer, must "
                         "drop remaining spans, num dropped spans = %d",
                         remaining_spans);
        for (int i = 0; i < remaining_spans; i++) {
            jaeger_span_protobuf_destroy(batch->spans[batch->n_spans + i]);
            jaeger_free(batch->spans[batch->n_spans + i]);
        }
        if (metrics != NULL) {
            jaeger_counter* dropped = metrics->reporter_dropped;
            assert(dropped != NULL);
            dropped->inc(dropped, remaining_spans);
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

    /* Try to flush any spans we have not flushed yet. */
    ((jaeger_reporter*) r)->flush((jaeger_reporter*) r);

    if (r->fd >= 0) {
        close(r->fd);
        r->fd = -1;
    }

    if (r->candidates != NULL) {
        freeaddrinfo(r->candidates);
        r->candidates = NULL;
    }

    process_destroy(&r->process);

    for (int i = 0, len = jaeger_vector_length(&r->spans); i < len; i++) {
        Jaegertracing__Protobuf__Span** span =
            jaeger_vector_offset(&r->spans, i);
        assert(span != NULL);
        if (*span == NULL) {
            continue;
        }
        jaeger_span_protobuf_destroy(*span);
        jaeger_free(*span);
    }
    jaeger_vector_destroy(&r->spans);

    jaeger_mutex_destroy(&r->mutex);
}

static inline void
remote_reporter_update_queue_length(jaeger_remote_reporter* reporter)
{
    assert(reporter != NULL);
    if (reporter->metrics == NULL) {
        return;
    }
    jaeger_counter* dropped = reporter->metrics->reporter_dropped;
    assert(dropped != NULL);
    dropped->inc(dropped, 1);
}

static void remote_reporter_report(jaeger_reporter* reporter,
                                   const jaeger_span* span)
{
    assert(reporter != NULL);
    if (span == NULL) {
        return;
    }

    jaeger_remote_reporter* r = (jaeger_remote_reporter*) reporter;
    jaeger_mutex_lock(&r->mutex);

    if (!jaeger_vector_reserve(&r->spans,
                               jaeger_vector_length(&r->spans) + 1)) {
        goto unlock;
    }

    Jaegertracing__Protobuf__Span* span_copy =
        jaeger_malloc(sizeof(Jaegertracing__Protobuf__Span));
    if (span_copy == NULL) {
        jaeger_log_error("Cannot allocate span for reporter batch");
        goto unlock;
    }

    *span_copy =
        (Jaegertracing__Protobuf__Span) JAEGERTRACING__PROTOBUF__SPAN__INIT;
    if (!jaeger_span_to_protobuf(span_copy, span)) {
        goto cleanup;
    }
    Jaegertracing__Protobuf__Span** span_ptr = jaeger_vector_append(&r->spans);
    assert(span_ptr != NULL);
    *span_ptr = span_copy;
    remote_reporter_update_queue_length(r);

    if (r->process.service_name == NULL ||
        strlen(r->process.service_name) == 0) {
        /* Building process will not affect this span, so ignore return value.
         */
        build_process(&r->process, span->tracer);
    }

    goto unlock;

cleanup:
    jaeger_span_protobuf_destroy(span_copy);
    jaeger_free(span_copy);
unlock:
    jaeger_mutex_unlock(&r->mutex);
}

static bool remote_reporter_write_to_socket(jaeger_remote_reporter* reporter,
                                            const uint8_t* buffer,
                                            int buf_len)
{
    /* Delay address resolution until the first write. */
    if (reporter->candidates != NULL) {
        bool success = false;
        for (struct addrinfo* iter = reporter->candidates; iter != NULL;
             iter = iter->ai_next) {
            if (sendto(reporter->fd,
                       buffer,
                       buf_len,
                       0,
                       iter->ai_addr,
                       iter->ai_addrlen) == buf_len &&
                sizeof(reporter->addr) == iter->ai_addrlen) {
                memcpy(&reporter->addr, iter->ai_addr, sizeof(reporter->addr));
                success = true;
                break;
            }
        }

        freeaddrinfo(reporter->candidates);
        reporter->candidates = NULL;
        if (!success) {
            jaeger_log_error("Failed to resolve remote reporter host port");
            if (reporter->metrics != NULL) {
                jaeger_counter* failed = reporter->metrics->reporter_failure;
                assert(failed != NULL);
                failed->inc(failed, 1);
            }
        }

        return success;
    }

    const int num_written = sendto(reporter->fd,
                                   buffer,
                                   buf_len,
                                   0,
                                   (struct sockaddr*) &reporter->addr,
                                   sizeof(reporter->addr));
    const bool success = (num_written == buf_len);
    if (!success) {
        jaeger_log_error("Cannot write entire message to UDP socket, "
                         "num written = %d, errno = %d",
                         num_written,
                         errno);
        if (reporter->metrics != NULL) {
            jaeger_counter* failed = reporter->metrics->reporter_failure;
            assert(failed != NULL);
            failed->inc(failed, 1);
        }
    }

    return success;
}

static bool remote_reporter_flush_batch(jaeger_remote_reporter* reporter)
{
    const int num_spans = jaeger_vector_length(&reporter->spans);
    assert(num_spans > 0);

    Jaegertracing__Protobuf__Batch batch = JAEGERTRACING__PROTOBUF__BATCH__INIT;
    if (!build_batch(&batch,
                     &reporter->process,
                     &reporter->spans,
                     reporter->max_packet_size,
                     reporter->metrics)) {
        return false;
    }

    uint8_t buffer[reporter->max_packet_size];
    ProtobufCBufferSimple simple = PROTOBUF_C_BUFFER_SIMPLE_INIT(buffer);
    jaegertracing__protobuf__batch__pack_to_buffer(&batch,
                                                   (ProtobufCBuffer*) &simple);
    assert((int) simple.len <= reporter->max_packet_size);
    const bool write_succeeded =
        remote_reporter_write_to_socket(reporter, simple.data, simple.len);
    PROTOBUF_C_BUFFER_SIMPLE_CLEAR(&simple);
    if (!write_succeeded) {
        goto cleanup;
    }
    const int num_flushed = batch.n_spans;
    for (int i = 0; i < num_flushed; i++) {
        if (batch.spans[i] == NULL) {
            continue;
        }
        jaeger_span_protobuf_destroy(batch.spans[i]);
        jaeger_free(batch.spans[i]);
    }
    jaeger_free(batch.spans);
    if (reporter->metrics != NULL) {
        jaeger_counter* num_success = reporter->metrics->reporter_success;
        assert(num_success != NULL);
        num_success->inc(num_success, num_flushed);
    }
    remote_reporter_update_queue_length(reporter);
    return true;

cleanup:
    if (reporter->spans.data != NULL) {
        /* No need to delete the span pointers because they are still valid
         * and in batch's buffer. */
        jaeger_free(reporter->spans.data);
        reporter->spans.data = NULL;
    }
    reporter->spans.data = (char*) batch.spans;
    reporter->spans.len = num_spans;
    remote_reporter_update_queue_length(reporter);
    return false;
}

static bool remote_reporter_flush(jaeger_reporter* r)
{
    assert(r != NULL);

    jaeger_remote_reporter* reporter = (jaeger_remote_reporter*) r;
    bool success = true;
    jaeger_mutex_lock(&reporter->mutex);
    for (int num_spans = jaeger_vector_length(&reporter->spans); num_spans > 0;
         num_spans = jaeger_vector_length(&reporter->spans)) {
        if (!remote_reporter_flush_batch(reporter)) {
            success = false;
            break;
        }
    }
    jaeger_mutex_unlock(&reporter->mutex);
    return success;
}

bool jaeger_remote_reporter_init(jaeger_remote_reporter* reporter,
                                 const char* host_port_str,
                                 int max_packet_size,
                                 jaeger_metrics* metrics)
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
                            sizeof(Jaegertracing__Protobuf__Span*))) {
        goto cleanup;
    }

    jaeger_host_port host_port =
        (jaeger_host_port) JAEGERTRACINGC_HOST_PORT_INIT;
    if (host_port_str == NULL || strlen(host_port_str) == 0) {
        host_port_str = JAEGERTRACINGC_DEFAULT_UDP_SPAN_SERVER_HOST_PORT;
    }
    if (!jaeger_host_port_scan(&host_port, host_port_str)) {
        goto cleanup_host_port;
    }

    reporter->candidates = NULL;
    struct addrinfo* candidates = NULL;
    memset(&reporter->addr, 0, sizeof(reporter->addr));
    if (!jaeger_host_port_resolve(&host_port, SOCK_DGRAM, &candidates)) {
        goto cleanup_host_port;
    }
    reporter->candidates = candidates;

    jaeger_host_port_destroy(&host_port);
    reporter->metrics = metrics;
    ((jaeger_destructible*) reporter)->destroy = &remote_reporter_destroy;
    ((jaeger_reporter*) reporter)->report = &remote_reporter_report;
    ((jaeger_reporter*) reporter)->flush = &remote_reporter_flush;
    reporter->mutex = (jaeger_mutex) JAEGERTRACINGC_MUTEX_INIT;
    return true;

cleanup_host_port:
    jaeger_host_port_destroy(&host_port);
cleanup:
    remote_reporter_destroy((jaeger_destructible*) reporter);
    return false;
}
