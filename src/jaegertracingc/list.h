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

/**
 * @file Linked list implementation.
 */

#ifndef JAEGERTRACINGC_LIST_H
#define JAEGERTRACINGC_LIST_H

#include "jaegertracingc/common.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * Base class for list elems.
 * @extends jaeger_destructible
 */
typedef struct jaeger_list_node {
    /** Base class member. */
    jaeger_destructible base;
    /** Pointer to previous node. */
    struct jaeger_list_node* prev;
    /** Pointer to next node. */
    struct jaeger_list_node* next;
} jaeger_list_node;

/** Convenience macro to implement list node subclass. */
#define JAEGERTRACINGC_DEFINE_LIST_NODE(node_type_name, data_type, destructor) \
    typedef struct node_type_name {                                            \
        jaeger_list_node base;                                                 \
        data_type data;                                                        \
    } node_type_name;                                                          \
                                                                               \
    static inline void node_type_name##_dealloc(jaeger_destructible* d)        \
    {                                                                          \
        destructor(&((node_type_name*) d)->data);                              \
        jaeger_free(d);                                                        \
    }                                                                          \
                                                                               \
    static inline node_type_name* node_type_name##_new(data_type data)         \
    {                                                                          \
        node_type_name* node =                                                 \
            (node_type_name*) jaeger_malloc(sizeof(node_type_name));           \
        if (node == NULL) {                                                    \
            return NULL;                                                       \
        }                                                                      \
        memset(node, 0, sizeof(*node));                                        \
        ((jaeger_destructible*) node)->destroy = &node_type_name##_dealloc;    \
        node->data = data;                                                     \
        return node;                                                           \
    }

/** Linked list implementation. */
typedef struct jaeger_list {
    /** Head of list. */
    jaeger_list_node* head;
    /** Size of list. Avoids counting via traversing. */
    size_t size;
} jaeger_list;

#define JAEGERTRACINGC_LIST_INIT \
    {                            \
        .head = NULL, .size = 0  \
    }

static inline jaeger_list_node* jaeger_list_get(jaeger_list* list, size_t index)
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

static inline void
jaeger_list_insert(jaeger_list* list, size_t index, jaeger_list_node* elem)
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

static inline void jaeger_list_append(jaeger_list* list, jaeger_list_node* elem)
{
    return jaeger_list_insert(list, list->size, elem);
}

static inline void jaeger_list_node_remove(jaeger_list* list,
                                           jaeger_list_node* node)
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

    ((jaeger_destructible*) node)->destroy((jaeger_destructible*) node);
    list->size--;
}

static inline void jaeger_list_clear(jaeger_list* list)
{
    if (list == NULL) {
        return;
    }

    for (jaeger_list_node *node = list->head, *next_node; node != NULL;
         node = next_node) {
        next_node = node->next;
        ((jaeger_destructible*) node)->destroy((jaeger_destructible*) node);
        list->size--;
    }

    list->head = NULL;
    assert(list->size == 0);
}

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* JAEGERTRACINGC_LIST_H */
