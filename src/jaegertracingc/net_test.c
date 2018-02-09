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

#include "jaegertracingc/net.h"
#include "unity.h"

void test_net()
{
    jaeger_host_port host_port;
    TEST_ASSERT_FALSE(jaeger_host_port_init(&host_port, NULL, 0));
    TEST_ASSERT_FALSE(jaeger_host_port_init(&host_port, "", 0));
    TEST_ASSERT_FALSE(jaeger_host_port_init(&host_port, "localhost", -1));
    TEST_ASSERT_TRUE(jaeger_host_port_init(&host_port, "localhost", 0));

    char buffer[strlen(host_port.host) + 1];
    TEST_ASSERT_EQUAL(
        strlen(host_port.host),
        jaeger_host_port_format(&host_port, buffer, sizeof(buffer)));
    TEST_ASSERT_EQUAL_STRING("localhost", buffer);

    jaeger_host_port_destroy(&host_port);

    jaeger_url url;
    url.str = "";
    TEST_ASSERT_FALSE(jaeger_host_port_from_url(&host_port, &url));
}
