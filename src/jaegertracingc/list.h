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
 * Base class for list elements.
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

jaeger_list_node* jaeger_list_get(jaeger_list* list, size_t index);

void jaeger_list_insert(jaeger_list* list,
                        size_t index,
                        jaeger_list_node* elem);

void jaeger_list_append(jaeger_list* list, jaeger_list_node* elem);

void jaeger_list_node_remove(jaeger_list* list, jaeger_list_node* node);

void jaeger_list_clear(jaeger_list* list);

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* JAEGERTRACINGC_LIST_H */
