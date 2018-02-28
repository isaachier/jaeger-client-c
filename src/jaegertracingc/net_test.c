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

    jaeger_url url = JAEGERTRACINGC_URL_INIT;
    TEST_ASSERT_FALSE(jaeger_url_init(&url, "test"));
    url.str = "";
    memset(&url.parts, 0, sizeof(url.parts));
    TEST_ASSERT_TRUE(jaeger_host_port_from_url(&host_port, &url));
    jaeger_host_port_destroy(&host_port);

    TEST_ASSERT_FALSE(jaeger_host_port_scan(&host_port, NULL));
    TEST_ASSERT_FALSE(jaeger_host_port_scan(&host_port, ""));
    TEST_ASSERT_FALSE(jaeger_host_port_scan(&host_port, ":"));
    TEST_ASSERT_FALSE(jaeger_host_port_scan(&host_port, "test:me"));
    TEST_ASSERT_TRUE(jaeger_host_port_scan(&host_port, ":5678"));
    jaeger_host_port_destroy(&host_port);
    TEST_ASSERT_TRUE(jaeger_host_port_scan(&host_port, "localhost"));
    jaeger_host_port_destroy(&host_port);
    TEST_ASSERT_TRUE(jaeger_host_port_scan(&host_port, "localhost:5678"));
    jaeger_host_port_destroy(&host_port);

    host_port.host = "localhost";
    host_port.port = 0;
    struct addrinfo* addrs = NULL;
    TEST_ASSERT_FALSE(jaeger_host_port_resolve(&host_port, 123, &addrs));
}
