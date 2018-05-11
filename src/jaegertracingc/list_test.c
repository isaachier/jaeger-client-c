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

#include "jaegertracingc/list.h"
#include "unity.h"

JAEGERTRACINGC_DEFINE_LIST_NODE(number_node, int)

void test_list()
{
    jaeger_list number_list = JAEGERTRACINGC_LIST_INIT(number_list);
    for (int i = 0; i < 10; i++) {
        jaeger_list_node* elem = (jaeger_list_node*) number_node_new(i);
        TEST_ASSERT_NOT_NULL(elem);
        jaeger_list_append(&number_list, elem);
        const number_node* node =
            (const number_node*) jaeger_list_get(&number_list, i);
        TEST_ASSERT_EQUAL(i, node->data);
    }
    jaeger_list_clear(&number_list);
}
