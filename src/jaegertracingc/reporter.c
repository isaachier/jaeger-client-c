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
    if (destructible != NULL) {
        jaeger_in_memory_reporter* r =
            (jaeger_in_memory_reporter*) destructible;
        jaeger_vector_destroy(&r->spans);
        jaeger_mutex_destroy(&r->mutex);
    }
}

static void in_memory_reporter_report(jaeger_reporter* reporter,
                                      const jaeger_span* span,
                                      jaeger_logger* logger)
{
    assert(reporter != NULL);
    jaeger_in_memory_reporter* r = (jaeger_in_memory_reporter*) reporter;
    if (span != NULL) {
        jaeger_mutex_lock(&r->mutex);
        jaeger_span* span_copy = jaeger_vector_append(&r->spans, NULL);
        if (span_copy != NULL) {
            jaeger_span_copy(span_copy, span, logger);
        }
        jaeger_mutex_unlock(&r->mutex);
    }
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
    if (destructible != NULL) {
        jaeger_composite_reporter* r =
            (jaeger_composite_reporter*) destructible;

#define DESTROY(x)                   \
    do {                             \
        if (x == NULL) {             \
            continue;                \
        }                            \
        jaeger_destructible** d = x; \
        if (*d == NULL) {            \
            continue;                \
        }                            \
        (*d)->destroy(*d);           \
        jaeger_free(*d);             \
    } while (0)

        JAEGERTRACINGC_VECTOR_FOR_EACH(&r->reporters, DESTROY);

#undef DESTROY
    }
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

static void remote_reporter_destroy(jaeger_destructible* destructible)
{
    if (destructible != NULL) {
        jaeger_remote_reporter* r = (jaeger_remote_reporter*) destructible;
        if (r->fd >= 0) {
            close(r->fd);
        }
    }
}

typedef Jaegertracing__Protobuf__Process jaeger_process;

static inline void jaeger_process_destroy(jaeger_process* process)
{
    if (process != NULL) {
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
        if (process->service_name != NULL) {
            jaeger_free(process->service_name);
            process->service_name = NULL;
        }
    }
}

static inline jaeger_process* build_process(const jaeger_tracer* tracer,
                                            jaeger_logger* logger)
{
    assert(tracer != NULL);
    Jaegertracing__Protobuf__Process* process =
        jaeger_malloc(sizeof(Jaegertracing__Protobuf__Process));
    if (process == NULL) {
        if (logger != NULL) {
            logger->error(logger,
                          "Cannot allocate process for batch message, "
                          "size of process = %d",
                          sizeof(Jaegertracing__Protobuf__Process));
        }
        return NULL;
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
    return process;

cleanup:
    jaeger_process_destroy(process);
    jaeger_free(process);
    return NULL;
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
        return;
    }

    Jaegertracing__Protobuf__Span* span_copy =
        jaeger_malloc(sizeof(Jaegertracing__Protobuf__Span));
    if (span_copy == NULL) {
        logger->error(logger, "Cannot allocate span for reporter batch");
        return;
    }
    *span_copy =
        (Jaegertracing__Protobuf__Span) JAEGERTRACING__PROTOBUF__SPAN__INIT;
    if (!jaeger_span_to_protobuf(span_copy, span, logger)) {
        goto cleanup;
    }

    if (r->batch.process == NULL) {
        jaeger_mutex_lock((jaeger_mutex*) &span->mutex);
        r->batch.process = build_process(span->tracer, logger);
        jaeger_mutex_unlock((jaeger_mutex*) &span->mutex);
        if (r->batch.process == NULL) {
            goto cleanup;
        }
    }

    return;

cleanup:
    jaeger_free(span_copy);
}

#define DEFAULT_UDP_BUFFER_SIZE USHRT_MAX

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

    reporter->batch =
        (Jaegertracing__Protobuf__Batch) JAEGERTRACING__PROTOBUF__BATCH__INIT;
    reporter->max_packet_size =
        (max_packet_size > 0) ? max_packet_size : DEFAULT_UDP_BUFFER_SIZE;
    if (!jaeger_vector_init(&reporter->spans,
                            sizeof(Jaegertracing__Protobuf__Span*),
                            NULL,
                            logger)) {
        goto cleanup_fd;
    }

    jaeger_host_port host_port =
        (jaeger_host_port) JAEGERTRACINGC_HOST_PORT_INIT;
    if (host_port_str == NULL || strlen(host_port_str) == 0) {
        host_port_str = JAEGERTRACINGC_DEFAULT_UDP_SPAN_SERVER_HOST_PORT;
    }
    if (!jaeger_host_port_scan(&host_port, host_port_str, logger)) {
        goto cleanup_spans;
    }
    struct addrinfo* addrs;
    if (!jaeger_host_port_resolve(&host_port, SOCK_DGRAM, &addrs, logger)) {
        goto cleanup_host_port;
    }
    bool success = false;
    for (struct addrinfo* iter = addrs; iter != NULL; iter = iter->ai_next) {
        if (connect(fd, iter->ai_addr, iter->ai_addrlen) == 0) {
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
        goto cleanup_host_port;
    }
    jaeger_host_port_destroy(&host_port);
    reporter->metrics = metrics;
    reporter->destroy = &remote_reporter_destroy;
    reporter->report = &remote_reporter_report;
    return true;

cleanup_host_port:
    jaeger_host_port_destroy(&host_port);
cleanup_spans:
    jaeger_vector_destroy(&reporter->spans);
cleanup_fd:
    close(fd);
    return false;
}

bool jaeger_remote_reporter_flush(jaeger_remote_reporter* reporter,
                                  jaeger_logger* logger)
{
    assert(reporter != NULL);
    /* TODO */
    return true;
}
