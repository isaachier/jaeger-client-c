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
        TEST_ASSERT_EQUAL(i + 1, number_list.size);
    }
    number_node* node = (number_node*) jaeger_list_get(&number_list, 4);
    TEST_ASSERT_NOT_NULL(node);
    TEST_ASSERT_EQUAL(4, node->data);
    const number_node* prev =
        (const number_node*) ((const jaeger_list_node*) node)->prev;
    TEST_ASSERT_NOT_NULL(prev);
    TEST_ASSERT_EQUAL_PTR(node, ((const jaeger_list_node*) prev)->next);
    TEST_ASSERT_EQUAL(3, prev->data);
    const number_node* next =
        (const number_node*) ((const jaeger_list_node*) node)->next;
    TEST_ASSERT_NOT_NULL(next);
    TEST_ASSERT_EQUAL(5, next->data);

    jaeger_list_node_remove(&number_list, (jaeger_list_node*) node);
    TEST_ASSERT_EQUAL_PTR(prev, ((const jaeger_list_node*) next)->prev);
    TEST_ASSERT_EQUAL_PTR(next, ((const jaeger_list_node*) prev)->next);
    TEST_ASSERT_EQUAL(9, number_list.size);

    jaeger_list_clear(&number_list);
    TEST_ASSERT_EQUAL(0, number_list.size);
}
