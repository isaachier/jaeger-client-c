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

#include <unistd.h>

#ifdef HOST_NAME_MAX
#define HOST_NAME_MAX_LEN HOST_NAME_MAX + 1
#else
#define HOST_NAME_MAX_LEN 256
#endif /* HOST_NAME_MAX */

static inline void append_hostname_tag(jaeger_tracer* tracer)
{
    assert(tracer != NULL);
    char hostname[HOST_NAME_MAX_LEN] = {'\0'};
    if (gethostname(hostname, HOST_NAME_MAX_LEN) != 0) {
        return;
    }
    jaeger_tag* tag = jaeger_vector_append(&tracer->tags);
    if (tag == NULL) {
        return;
    }

    *tag = (jaeger_tag) JAEGERTRACINGC_TAG_INIT;
    if (!jaeger_tag_init(tag, JAEGERTRACINGC_TRACER_HOSTNAME_TAG_KEY)) {
        goto cleanup;
    }
    tag->value_case = JAEGERTRACINGC_TAG_TYPE(STR);
    tag->str_value = jaeger_strdup(hostname);
    if (tag->str_value == NULL) {
        goto cleanup;
    }
    return;

cleanup:
    jaeger_tag_destroy(tag);
    tracer->tags.len--;
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

    append_hostname_tag(tracer);

    return true;
}
