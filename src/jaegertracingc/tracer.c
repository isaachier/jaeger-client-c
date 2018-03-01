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

#ifdef HOST_NAME_MAX
#define HOST_NAME_MAX_LEN HOST_NAME_MAX + 1
#else
#define HOST_NAME_MAX_LEN 256
#endif /* HOST_NAME_MAX */

static inline char* hostname()
{
    char hostname[HOST_NAME_MAX_LEN] = {'\0'};
    if (gethostname(hostname, HOST_NAME_MAX_LEN) != 0) {
        jaeger_log_warn("Cannot get hostname, errno = %d", errno);
        return NULL;
    }
    return strdup(hostname);
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
    if ((addr->ifa_flags & IFF_LOOPBACK) == 0 &&
        ip_addr->sin_addr.s_addr != htonl(INADDR_LOOPBACK)) {
        score += 100;
        if ((addr->ifa_flags & IFF_UP) != 0) {
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

bool jaeger_tracer_init(jaeger_tracer* tracer,
                        jaeger_sampler* sampler,
                        jaeger_reporter* reporter,
                        const jaeger_tracer_options* options)
{
    assert(tracer != NULL);
    assert(sampler != NULL);
    assert(reporter != NULL);

    if (options != NULL) {
        tracer->options = *options;
    }
    tracer->sampler = sampler;
    tracer->reporter = reporter;

    if (jaeger_vector_init(&tracer->tags, sizeof(jaeger_tag))) {
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
    }

finish:
    return true;
}

void jaeger_tracer_report_span(jaeger_tracer* tracer, jaeger_span* span)
{
    /* TODO */
    (void) tracer;
    (void) span;
}
