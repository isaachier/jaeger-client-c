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

jaeger_list_node* jaeger_list_get(jaeger_list* list, size_t index)
{
    assert(list != NULL);
    if (index >= list->size) {
        return NULL;
    }

    jaeger_list_node* node = list->head;
    for (size_t i = 0; i < index; i++) {
        node = node->next;
    }
    return node;
}

void jaeger_list_insert(jaeger_list* list, size_t index, jaeger_list_node* elem)
{
    assert(list != NULL);
    assert(elem != NULL);
    assert(index <= list->size);

    if (index == 0) {
        if (list->head == NULL) {
            elem->next = NULL;
            elem->prev = NULL;
        }
        else {
            assert(list->head->prev == NULL);
            elem->prev = NULL;
            elem->next = list->head;
            list->head->prev = elem;
        }
        list->head = elem;
        list->size++;
        return;
    }

    assert(index > 0);
    jaeger_list_node* node = jaeger_list_get(list, index - 1);
    assert(node != NULL);
    elem->next = node->next;
    if (elem->next != NULL) {
        elem->next->prev = elem;
    }
    node->next = elem;
    elem->prev = node;

    list->size++;
}

void jaeger_list_append(jaeger_list* list, jaeger_list_node* elem)
{
    jaeger_list_insert(list, list->size, elem);
}

void jaeger_list_node_remove(jaeger_list* list, jaeger_list_node* node)
{
    assert(list != NULL);
    assert(node != NULL);

    if (list->head == node) {
        if (node->next != NULL) {
            list->head = node->next;
        }
        else {
            list->head = NULL;
        }
    }

    if (node->next != NULL) {
        node->next->prev = node->prev;
    }
    if (node->prev != NULL) {
        node->prev->next = node->next;
    }
    node->next = NULL;
    node->prev = NULL;

    list->size--;
}

void jaeger_list_clear(jaeger_list* list)
{
    if (list == NULL) {
        return;
    }

    for (jaeger_list_node *node = list->head, *next_node; node != NULL;
         node = next_node) {
        next_node = node->next;
        ((jaeger_destructible*) node)->destroy((jaeger_destructible*) node);
    }

    list->head = NULL;
    list->size = 0;
}
