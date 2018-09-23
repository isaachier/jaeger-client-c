/*
 * Copyright (c) 2018 The Jaeger Authors.
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

#include <stdio.h>
#include <string.h>
#include "jaegertracingc/tag.h"
#include "unity.h"

void test_tag()
{
    jaeger_vector list;
    bool result = jaeger_vector_init(&list, sizeof(jaeger_tag));
    TEST_ASSERT_TRUE(result);

    for (int i = 0; i < 10; i++) {
        jaeger_tag tag = JAEGERTRACINGC_TAG_INIT;
        tag.key = "test1";
        tag.v_type = JAEGER__MODEL__VALUE_TYPE__BOOL;
        tag.v_bool = true;
        result = jaeger_tag_vector_append(&list, &tag);
        TEST_ASSERT_TRUE(result);

        tag.key = "test2";
        tag.v_type = JAEGER__MODEL__VALUE_TYPE__FLOAT64;
        tag.v_float64 = 0.12;
        result = jaeger_tag_vector_append(&list, &tag);
        TEST_ASSERT_TRUE(result);

        tag.key = "test3";
        tag.v_type = JAEGER__MODEL__VALUE_TYPE__INT64;
        tag.v_int64 = -1234567890;
        result = jaeger_tag_vector_append(&list, &tag);
        TEST_ASSERT_TRUE(result);

        tag.key = "test4";
        tag.v_type = JAEGER__MODEL__VALUE_TYPE__STRING;
        char buffer[12] = "hello world";
        tag.v_str = &buffer[0];
        result = jaeger_tag_vector_append(&list, &tag);
        TEST_ASSERT_TRUE(result);
        memset(&buffer, 0, sizeof(buffer));
        TEST_ASSERT_EQUAL_STRING("hello world",
                                 ((jaeger_tag*) jaeger_vector_get(
                                      &list, jaeger_vector_length(&list) - 1))
                                     ->v_str);

        tag.key = "test5";
        tag.v_type = JAEGER__MODEL__VALUE_TYPE__BINARY;
        char binary_buffer[12] = "hello world";
        tag.v_binary.data = (uint8_t*) &binary_buffer[0];
        tag.v_binary.len = sizeof(binary_buffer);
        result = jaeger_tag_vector_append(&list, &tag);
        TEST_ASSERT_TRUE(result);
        memset(&binary_buffer, 0, sizeof(binary_buffer));
        TEST_ASSERT_EQUAL_STRING(
            "hello world",
            (char*) ((jaeger_tag*) jaeger_vector_get(
                         &list, jaeger_vector_length(&list) - 1))
                ->v_binary.data);
    }

    JAEGERTRACINGC_VECTOR_FOR_EACH(&list, jaeger_tag_destroy, jaeger_tag);
    jaeger_vector_destroy(&list);

    jaeger_tag_destroy(NULL);
}
