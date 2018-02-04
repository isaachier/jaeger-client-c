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

#include "jaegertracingc/tag.h"
#include "unity.h"
#include <stdio.h>
#include <string.h>

void test_tag()
{
    jaeger_tag_list tags = {0};
    bool result = jaeger_tag_list_init(&tags);
    TEST_ASSERT_TRUE(result);

    for (int i = 0; i < 10; i++) {
        jaeger_tag tag = (jaeger_tag) JAEGERTRACING__PROTOBUF__TAG__INIT;
        tag.key = "test1";
        tag.value_case = JAEGERTRACING__PROTOBUF__TAG__VALUE_BOOL_VALUE;
        tag.bool_value = true;
        result = jaeger_tag_list_append(&tags, &tag);
        TEST_ASSERT_TRUE(result);

        tag.key = "test2";
        tag.value_case = JAEGERTRACING__PROTOBUF__TAG__VALUE_DOUBLE_VALUE;
        tag.double_value = 0.12;
        result = jaeger_tag_list_append(&tags, &tag);
        TEST_ASSERT_TRUE(result);

        tag.key = "test3";
        tag.value_case = JAEGERTRACING__PROTOBUF__TAG__VALUE_LONG_VALUE;
        tag.long_value = -1234567890;
        result = jaeger_tag_list_append(&tags, &tag);
        TEST_ASSERT_TRUE(result);

        tag.key = "test4";
        tag.value_case = JAEGERTRACING__PROTOBUF__TAG__VALUE_STR_VALUE;
        char buffer[12] = "hello world";
        tag.str_value = &buffer[0];
        result = jaeger_tag_list_append(&tags, &tag);
        TEST_ASSERT_TRUE(result);
        memset(&buffer, 0, sizeof(buffer));
        TEST_ASSERT_EQUAL_STRING("hello world",
                                 tags.tags[tags.size - 1].str_value);

        tag.key = "test5";
        tag.value_case = JAEGERTRACING__PROTOBUF__TAG__VALUE_BINARY_VALUE;
        char binary_buffer[12] = "hello world";
        tag.binary_value.data = (uint8_t*) &binary_buffer[0];
        tag.binary_value.len = sizeof(binary_buffer);
        result = jaeger_tag_list_append(&tags, &tag);
        TEST_ASSERT_TRUE(result);
        memset(&binary_buffer, 0, sizeof(binary_buffer));
        TEST_ASSERT_EQUAL_STRING(
            "hello world", (char*) tags.tags[tags.size - 1].binary_value.data);
    }

    jaeger_tag_list_destroy(&tags);
}
