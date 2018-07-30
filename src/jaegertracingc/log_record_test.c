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

#include "jaegertracingc/log_record.h"

#include <string.h>

#include "unity.h"

void test_log_record()
{
    jaeger_timestamp timestamp;
    jaeger_timestamp_now(&timestamp);
    enum { num_fields = 5 };
    opentracing_log_field fields[num_fields];
    for (int i = 0; i < num_fields; i++) {
        TEST_ASSERT_LESS_THAN(10, i);
        const int key_size = sizeof("key") + 1;
        fields[i].key = jaeger_malloc(key_size);
        TEST_ASSERT_NOT_NULL(fields[i].key);
        TEST_ASSERT_EQUAL(
            key_size - 1,
            snprintf((char*) fields[i].key, key_size, "key%d", i));
        fields[i].value = (opentracing_value){.type = opentracing_value_bool,
                                              .value = {.bool_value = true}};
    }

    opentracing_log_record ot_log_record = {
        .timestamp = timestamp, .fields = fields, .num_fields = num_fields};
    jaeger_log_record log_record;
    TEST_ASSERT_TRUE(
        jaeger_log_record_from_opentracing(&log_record, &ot_log_record));
    for (int i = 0; i < num_fields; i++) {
        jaeger_free((char*) ot_log_record.fields[i].key);
    }

    Jaeger__Model__Log log_record_pb;
    TEST_ASSERT_TRUE(
        jaeger_log_record_to_protobuf(&log_record_pb, &log_record));
    jaeger_log_record_protobuf_destroy_wrapper(&log_record_pb);

    /* Test Protobuf conversion allocation failure. */
    jaeger_set_allocator(jaeger_null_allocator());
    TEST_ASSERT_FALSE(
        jaeger_log_record_to_protobuf(&log_record_pb, &log_record));

    /* Test copy allocation failure. */
    jaeger_log_record log_record_copy = JAEGERTRACINGC_LOG_RECORD_INIT;
    TEST_ASSERT_FALSE(jaeger_log_record_copy(&log_record_copy, &log_record));

    /* Test OT conversion allocation failure. */
    TEST_ASSERT_FALSE(
        jaeger_log_record_from_opentracing(&log_record_copy, &ot_log_record));

    jaeger_set_allocator(jaeger_built_in_allocator());

    jaeger_log_record_destroy_wrapper(&log_record);
}
