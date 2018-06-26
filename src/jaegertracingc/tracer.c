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

#include "jaegertracingc/tracer.h"

#include <arpa/inet.h>
#include <errno.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <unistd.h>

#include "jaegertracingc/propagation.h"
#include "jaegertracingc/random.h"
#include "jaegertracingc/span.h"

#ifdef HOST_NAME_MAX
#define HOST_NAME_MAX_LEN (HOST_NAME_MAX + 1)
#else
#define HOST_NAME_MAX_LEN 256
#endif /* HOST_NAME_MAX */

#define SAMPLING_PRIORITY_TAG_KEY "sampling.priority"

static inline char* hostname()
{
    char hostname[HOST_NAME_MAX_LEN] = {'\0'};
    if (gethostname(hostname, HOST_NAME_MAX_LEN) != 0) {
        jaeger_log_warn("Cannot get hostname, errno = %d", errno);
        return NULL;
    }
    return jaeger_strdup(hostname);
}

static inline int score_addr(const struct ifaddrs* addr)
{
    assert(addr != NULL);
    int score = 0;
    const struct sockaddr* sock_addr = addr->ifa_addr;
    if (sock_addr == NULL) {
        return 0;
    }
    if (sock_addr->sa_family == AF_INET) {
        score += 300;
    }
    struct sockaddr_in* ip_addr = (struct sockaddr_in*) sock_addr;
    if ((addr->ifa_flags & ((uint32_t) IFF_LOOPBACK)) == 0 &&
        ip_addr->sin_addr.s_addr != htonl(INADDR_LOOPBACK)) {
        score += 100;
        if ((addr->ifa_flags & ((uint32_t) IFF_UP)) != 0) {
            score += 100;
        }
    }
    return score;
}

static inline char* local_ip_str()
{
    struct ifaddrs* addrs = NULL;
    if (getifaddrs(&addrs) != 0) {
        jaeger_log_warn("Cannot get local IP, errno = %d", errno);
        return NULL;
    }

    int max_score = 0;
    struct ifaddrs* best_addr = NULL;
    for (struct ifaddrs* iter = addrs; iter != NULL; iter = iter->ifa_next) {
        const int score = score_addr(iter);
        if (score > max_score) {
            best_addr = iter;
            max_score = score;
        }
    }
    char* result = NULL;
    if (best_addr != NULL && best_addr->ifa_addr != NULL) {
        int max_host = 0;
        int port = 0;
        const void* src = NULL;
        if (best_addr->ifa_addr->sa_family == AF_INET) {
            max_host = INET_ADDRSTRLEN;
            const struct sockaddr_in* addr =
                (const struct sockaddr_in*) best_addr->ifa_addr;
            src = (const void*) &addr->sin_addr;
            port = htons(addr->sin_port);
        }
        else {
            max_host = INET6_ADDRSTRLEN;
            const struct sockaddr_in6* addr =
                (const struct sockaddr_in6*) best_addr->ifa_addr;
            src = (const void*) &addr->sin6_addr;
            port = htons(addr->sin6_port);
        }

        char buffer[max_host + 1 + JAEGERTRACINGC_MAX_PORT_STR_LEN];
        if (inet_ntop(
                best_addr->ifa_addr->sa_family, src, buffer, sizeof(buffer)) ==
            NULL) {
            jaeger_log_warn("Cannot convert IP address to string, errno = %d",
                            errno);
            goto cleanup;
        }

        const int len = strlen(buffer);
        const int rem_len = sizeof(buffer) - len;
        const int num_chars = snprintf(&buffer[len], rem_len, ":%d", port);
        if (num_chars <= rem_len) {
            result = strdup(buffer);
        }
    }

cleanup:
    freeifaddrs(addrs);
    return result;
}

static inline bool append_tag(jaeger_vector* tags, const char* key, char* value)
{
    if (value == NULL) {
        return false;
    }
    jaeger_tag* tag = jaeger_vector_append(tags);
    if (tag == NULL) {
        goto cleanup;
    }
    *tag = (jaeger_tag) JAEGERTRACINGC_TAG_INIT;
    if (!jaeger_tag_init(tag, (key))) {
        goto cleanup;
    }
    tag->value_case = JAEGERTRACINGC_TAG_TYPE(STR);
    tag->str_value = value;
    return true;

cleanup:
    jaeger_free(value);
    jaeger_tag_destroy(tag);
    tags->len--;
    return false;
}

static inline jaeger_metrics* default_metrics()
{
    /* TODO: Reconsider this in favor of null metrics like Go client does. */
    jaeger_metrics* metrics = jaeger_malloc(sizeof(jaeger_metrics));
    if (metrics == NULL) {
        jaeger_log_error("Cannot allocate default metrics");
        return NULL;
    }
    if (!jaeger_default_metrics_init(metrics)) {
        jaeger_log_error("Cannot initialize default metrics");
        return NULL;
    }
    return metrics;
}

static inline jaeger_sampler* default_sampler(const char* service_name,
                                              jaeger_metrics* metrics)
{
    jaeger_remotely_controlled_sampler* remote_sampler =
        jaeger_malloc(sizeof(jaeger_remotely_controlled_sampler));
    if (remote_sampler == NULL) {
        jaeger_log_error("Cannot allocate default sampler");
        return NULL;
    }
    if (!jaeger_remotely_controlled_sampler_init(
            remote_sampler, service_name, NULL, NULL, 0, metrics)) {
        jaeger_log_error("Cannot initialize default sampler");
        jaeger_free(remote_sampler);
        return NULL;
    }
    return (jaeger_sampler*) remote_sampler;
}

static inline jaeger_reporter* default_reporter(jaeger_metrics* metrics)
{
    jaeger_remote_reporter* reporter =
        jaeger_malloc(sizeof(jaeger_remote_reporter));
    if (reporter == NULL) {
        jaeger_log_error("Cannot allocate default reporter");
        return NULL;
    }
    if (!jaeger_remote_reporter_init(reporter, NULL, 0, metrics)) {
        jaeger_log_error("Cannot initialize default reporter");
        jaeger_free(reporter);
        return NULL;
    }
    return (jaeger_reporter*) reporter;
}

opentracing_span* jaeger_tracer_start_span(opentracing_tracer* tracer,
                                           const char* operation_name)
{
    return jaeger_tracer_start_span_with_options(tracer, operation_name, NULL);
}

void jaeger_tracer_close(opentracing_tracer* tracer)
{
    jaeger_tracer_flush((jaeger_tracer*) tracer);
}

void jaeger_tracer_destroy(jaeger_destructible* d)
{
    if (d == NULL) {
        return;
    }

    jaeger_tracer* tracer = (jaeger_tracer*) d;
    ((opentracing_tracer*) tracer)->close(((opentracing_tracer*) tracer));
    if (tracer->service_name != NULL) {
        jaeger_free(tracer->service_name);
        tracer->service_name = NULL;
    }

    if (tracer->metrics != NULL) {
        jaeger_metrics_destroy(tracer->metrics);
        if (tracer->allocated.metrics) {
            jaeger_free(tracer->metrics);
            tracer->allocated.metrics = false;
        }
        tracer->metrics = NULL;
    }

    if (tracer->sampler != NULL) {
        ((jaeger_destructible*) tracer->sampler)
            ->destroy((jaeger_destructible*) tracer->sampler);
        if (tracer->allocated.sampler) {
            jaeger_free(tracer->sampler);
            tracer->allocated.sampler = false;
        }
        tracer->sampler = NULL;
    }

    if (tracer->reporter != NULL) {
        ((jaeger_destructible*) tracer->reporter)
            ->destroy((jaeger_destructible*) tracer->reporter);
        if (tracer->allocated.reporter) {
            jaeger_free(tracer->reporter);
            tracer->allocated.reporter = false;
        }
        tracer->reporter = NULL;
    }

    JAEGERTRACINGC_VECTOR_FOR_EACH(
        &tracer->tags, jaeger_tag_destroy, jaeger_tag);
    jaeger_vector_destroy(&tracer->tags);
}

bool jaeger_tracer_init(jaeger_tracer* tracer,
                        const char* service_name,
                        jaeger_sampler* sampler,
                        jaeger_reporter* reporter,
                        jaeger_metrics* metrics,
                        const jaeger_tracer_options* options,
                        const jaeger_headers_config* headers)
{
    assert(tracer != NULL);

    tracer->service_name = jaeger_strdup(service_name);
    if (tracer->service_name == NULL) {
        goto cleanup;
    }

    if (metrics == NULL) {
        tracer->metrics = default_metrics();
        if (tracer->metrics == NULL) {
            goto cleanup;
        }
        tracer->allocated.metrics = true;
    }
    else {
        tracer->metrics = metrics;
        tracer->allocated.metrics = false;
    }

    if (sampler == NULL) {
        tracer->sampler = default_sampler(service_name, metrics);
        if (tracer->sampler == NULL) {
            goto cleanup;
        }
        tracer->allocated.sampler = true;
    }
    else {
        tracer->sampler = sampler;
        tracer->allocated.sampler = false;
    }

    if (reporter == NULL) {
        tracer->reporter = default_reporter(metrics);
        if (tracer->reporter == NULL) {
            goto cleanup;
        }
        tracer->allocated.reporter = true;
    }
    else {
        tracer->reporter = reporter;
        tracer->allocated.reporter = false;
    }

    if (options != NULL) {
        tracer->options = *options;
    }

    if (headers != NULL) {
        tracer->headers = *headers;
    }

    if (!jaeger_vector_init(&tracer->tags, sizeof(jaeger_tag))) {
        /* If we run out of memory on tracer construction, we might as well
         * consider it a fatal error seeing as nothing will work. Strictly
         * speaking, the tags might be not critical to tracer execution, but it
         * is not worth considering the edge case given the clear memory issues
         * this case would imply.
         */
        return false;
    }

    if (!append_tag(&tracer->tags,
                    JAEGERTRACINGC_CLIENT_VERSION_TAG_KEY,
                    strdup(JAEGERTRACINGC_CLIENT_VERSION))) {
        goto finish;
    }

    if (!append_tag(&tracer->tags,
                    JAEGERTRACINGC_TRACER_HOSTNAME_TAG_KEY,
                    hostname())) {
        goto finish;
    }

    if (!append_tag(
            &tracer->tags, JAEGERTRACINGC_TRACER_IP_TAG_KEY, local_ip_str())) {
        goto finish;
    }

finish:
    return true;

cleanup:
    jaeger_tracer_destroy((jaeger_destructible*) tracer);
    return false;
}

static inline bool
span_inherit_from_parent(jaeger_tracer* tracer,
                         jaeger_span* span,
                         const opentracing_span_reference* span_refs,
                         int num_span_refs)
{
    assert(tracer != NULL);
    assert(span != NULL);
    assert(span_refs != NULL);
    const jaeger_span_context* parent = NULL;
    bool has_parent = false;
    for (int i = 0; i < num_span_refs; i++) {
        const opentracing_span_reference* span_ref = &span_refs[i];
        const jaeger_span_context* ctx =
            (const jaeger_span_context*) span_ref->referenced_context;
        jaeger_mutex_lock((jaeger_mutex*) &ctx->mutex);
        const int num_baggage_items = ctx->baggage.size;
        jaeger_mutex_unlock((jaeger_mutex*) &ctx->mutex);
        if (!jaeger_span_context_is_valid(ctx) &&
            !jaeger_span_context_is_debug_id_container_only(ctx) &&
            num_baggage_items == 0) {
            continue;
        }
        jaeger_span_ref* span_ref_copy = jaeger_vector_append(&span->refs);
        if (span_ref_copy == NULL) {
            return false;
        }
        if (!jaeger_span_ref_copy(
                NULL, (void*) span_ref_copy, (const void*) span_ref)) {
            span->refs.len--;
            return false;
        }
        if (parent == NULL) {
            parent = ctx;
            has_parent =
                (span_ref->type == opentracing_span_reference_child_of);
        }
    }

    if (!has_parent && parent != NULL && jaeger_span_context_is_valid(parent)) {
        has_parent = true;
    }

    if (!has_parent || parent == NULL ||
        !jaeger_span_context_is_valid(parent)) {
        span->context.trace_id.low = jaeger_random64();
        if (tracer->options.gen_128_bit) {
            span->context.trace_id.high = jaeger_random64();
        }
        span->context.span_id = span->context.trace_id.low;
        span->context.parent_id = 0;
        span->context.flags = 0;
        if (has_parent &&
            jaeger_span_context_is_debug_id_container_only(parent)) {
            span->context.flags |= ((uint8_t) jaeger_sampling_flag_sampled |
                                    (uint8_t) jaeger_sampling_flag_debug);
            append_tag(
                &span->tags, JAEGERTRACINGC_DEBUG_HEADER, parent->debug_id);
        }
        else if (tracer->sampler->is_sampled(tracer->sampler,
                                             &span->context.trace_id,
                                             span->operation_name,
                                             &span->tags)) {
            span->context.flags |= jaeger_sampling_flag_sampled;
        }
    }
    else {
        span->context.trace_id = parent->trace_id;
        span->context.span_id = jaeger_random64();
        span->context.parent_id = parent->span_id;
        span->context.flags = parent->flags;
    }

    if (has_parent) {
        assert(parent != NULL);
        if (!jaeger_hashtable_copy(&span->context.baggage, &parent->baggage)) {
            return false;
        }
    }

    return true;
}

static inline void update_metrics_for_new_span(jaeger_metrics* metrics,
                                               const jaeger_span* span)
{
#define COUNTER_INCREMENT(counter) (counter)->inc((counter), 1)

    assert(metrics != NULL);
    assert(span != NULL);

    jaeger_counter* spans_started = metrics->spans_started;
    spans_started->inc(spans_started, 1);

    jaeger_mutex_lock((jaeger_mutex*) &span->mutex);
    const bool is_sampled = jaeger_span_is_sampled(span);
    const bool is_new_trace = (span->context.parent_id == 0);
    jaeger_mutex_unlock((jaeger_mutex*) &span->mutex);
    if (is_sampled) {
        COUNTER_INCREMENT(metrics->spans_sampled);
        if (is_new_trace) {
            COUNTER_INCREMENT(metrics->traces_started_sampled);
        }
    }
    else {
        COUNTER_INCREMENT(metrics->spans_sampled);
        if (is_new_trace) {
            COUNTER_INCREMENT(metrics->traces_started_not_sampled);
        }
    }

#undef COUNTER_INCREMENT
}

opentracing_span* jaeger_tracer_start_span_with_options(
    opentracing_tracer* tracer,
    const char* operation_name,
    const opentracing_start_span_options* options)
{
    assert(tracer != NULL);
    assert(operation_name != NULL);

    if (options == NULL) {
        opentracing_start_span_options opts = {.references = NULL,
                                               .num_references = 0,
                                               .tags = NULL,
                                               .num_tags = 0};
        jaeger_duration_now(&opts.start_time_steady);
        jaeger_timestamp_now(&opts.start_time_system);
        return jaeger_tracer_start_span_with_options(
            tracer, operation_name, &opts);
    }

    assert(options != NULL);
    assert(options->num_references >= 0);
    assert(options->references == NULL || options->num_references == 0);
    assert(options->num_tags >= 0);
    assert(options->tags == NULL || options->num_tags == 0);

    jaeger_tracer* t = (jaeger_tracer*) tracer;
    jaeger_span* span = jaeger_malloc(sizeof(jaeger_span));
    if (span == NULL) {
        jaeger_log_error("Cannot allocate span, operation name = %s",
                         operation_name);
        return NULL;
    }
    if (!jaeger_span_init(span) ||
        !span_inherit_from_parent(
            t, span, options->references, options->num_references)) {
        goto cleanup;
    }

    for (int i = 0; i < options->num_tags; i++) {
        assert(options->tags != NULL);
        jaeger_tag src_tag = JAEGERTRACINGC_TAG_INIT;
        jaeger_tag_from_key_value(
            &src_tag, options->tags[i].key, &options->tags[i].value);
        assert(src_tag.key != NULL);
        const opentracing_value value = {};

        if (strcmp(src_tag.key, SAMPLING_PRIORITY_TAG_KEY) == 0 &&
            src_tag.value_case == JAEGERTRACINGC_TAG_TYPE(LONG)) {
            if (jaeger_span_set_sampling_priority(span, &value)) {
                continue;
            }
        }

        jaeger_span_set_tag_no_locking(span, src_tag.key, &value);
    }

    span->tracer = t;
    span->operation_name = jaeger_strdup(operation_name);
    if (span->operation_name == NULL) {
        goto cleanup;
    }
    span->duration = (jaeger_duration) JAEGERTRACINGC_DURATION_INIT;

    if (options->start_time_system.value.tv_sec == 0 &&
        options->start_time_system.value.tv_nsec == 0) {
        jaeger_timestamp_now(&span->start_time_system);
    }
    else {
        span->start_time_system = options->start_time_system;
    }

    if (options->start_time_steady.value.tv_sec == 0 &&
        options->start_time_steady.value.tv_nsec == 0) {
        jaeger_duration_now(&span->start_time_steady);
    }
    else {
        span->start_time_steady = options->start_time_steady;
    }

    update_metrics_for_new_span(t->metrics, span);

    return (opentracing_span*) span;

cleanup:
    jaeger_span_destroy((jaeger_destructible*) span);
    jaeger_free(span);
    return NULL;
}

bool jaeger_tracer_flush(jaeger_tracer* tracer)
{
    assert(tracer != NULL);
    return tracer->reporter->flush(tracer->reporter);
}

void jaeger_tracer_report_span(jaeger_tracer* tracer, jaeger_span* span)
{
    jaeger_counter* spans_finished = tracer->metrics->spans_finished;
    spans_finished->inc(spans_finished, 1);
    if (jaeger_span_is_sampled(span)) {
        tracer->reporter->report(tracer->reporter, span);
    }
    /* TODO: pooling? */
}

#define CHECK_SPAN_CONTEXT(ctx)                                             \
    do {                                                                    \
        if (((int) (ctx)->type_descriptor_length) !=                        \
                ((int) jaeger_span_context_type_descriptor_length) ||       \
            memcmp((ctx)->type_descriptor,                                  \
                   jaeger_span_context_type_descriptor,                     \
                   jaeger_span_context_type_descriptor_length) != 0) {      \
            return opentracing_propagation_error_code_invalid_span_context; \
        }                                                                   \
    } while (0)

opentracing_propagation_error_code
jaeger_tracer_inject_text_map(opentracing_tracer* tracer,
                              opentracing_text_map_writer* writer,
                              const opentracing_span_context* span_context)
{
    CHECK_SPAN_CONTEXT(span_context);
    jaeger_tracer* t = (jaeger_tracer*) tracer;
    const jaeger_span_context* ctx = (jaeger_span_context*) span_context;
    return jaeger_inject_into_text_map(writer, ctx, &t->headers);
}

opentracing_propagation_error_code
jaeger_tracer_inject_http_headers(opentracing_tracer* tracer,
                                  opentracing_http_headers_writer* writer,
                                  const opentracing_span_context* span_context)
{
    CHECK_SPAN_CONTEXT(span_context);
    jaeger_tracer* t = (jaeger_tracer*) tracer;
    const jaeger_span_context* ctx = (jaeger_span_context*) span_context;
    return jaeger_inject_into_http_headers(writer, ctx, &t->headers);
}

opentracing_propagation_error_code
jaeger_tracer_inject_binary(opentracing_tracer* tracer,
                            int (*callback)(void*, const char*, size_t),
                            void* arg,
                            const opentracing_span_context* span_context)
{
    (void) tracer;
    CHECK_SPAN_CONTEXT(span_context);
    const jaeger_span_context* ctx = (jaeger_span_context*) span_context;
    return jaeger_inject_into_binary(callback, arg, ctx);
}

opentracing_propagation_error_code
jaeger_tracer_inject_custom(opentracing_tracer* tracer,
                            opentracing_custom_carrier_writer* carrier,
                            const opentracing_span_context* span_context)
{
    CHECK_SPAN_CONTEXT(span_context);
    jaeger_tracer* t = (jaeger_tracer*) tracer;
    const jaeger_span_context* ctx = (jaeger_span_context*) span_context;
    return jaeger_inject_into_custom(carrier, t, ctx);
}

#undef CHECK_SPAN_CONTEXT

opentracing_propagation_error_code
jaeger_tracer_extract_text_map(struct opentracing_tracer* tracer,
                               opentracing_text_map_reader* carrier,
                               opentracing_span_context** span_context)
{
    jaeger_tracer* t = (jaeger_tracer*) tracer;
    return jaeger_extract_from_text_map(
        carrier, (jaeger_span_context**) span_context, t->metrics, &t->headers);
}

opentracing_propagation_error_code
jaeger_tracer_extract_http_headers(opentracing_tracer* tracer,
                                   opentracing_http_headers_reader* carrier,
                                   opentracing_span_context** span_context)
{
    jaeger_tracer* t = (jaeger_tracer*) tracer;
    return jaeger_extract_from_http_headers(
        carrier, (jaeger_span_context**) span_context, t->metrics, &t->headers);
}

opentracing_propagation_error_code
jaeger_tracer_extract_binary(opentracing_tracer* tracer,
                             int (*callback)(void*, char*, size_t),
                             void* arg,
                             opentracing_span_context** span_context)
{
    jaeger_tracer* t = (jaeger_tracer*) tracer;
    return jaeger_extract_from_binary(
        callback, arg, (jaeger_span_context**) span_context, t->metrics);
}

opentracing_propagation_error_code
jaeger_tracer_extract_custom(opentracing_tracer* tracer,
                             opentracing_custom_carrier_reader* carrier,
                             opentracing_span_context** span_context)
{
    return carrier->extract(carrier, tracer, span_context);
}
