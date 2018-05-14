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
 * @file
 * Siphash implementation.
 * Based on https://github.com/veorq/SipHash/blob/master/siphash.c.
 */

#include "jaegertracingc/siphash.h"

#define C_ROUNDS 2
#define D_ROUNDS 4

static inline uint64_t unpack64(const uint8_t* buffer)
{
    return ((uint64_t) buffer[0]) | ((uint64_t) buffer[1] << 8u) |
           ((uint64_t) buffer[2] << 16u) | ((uint64_t) buffer[3] << 24u) |
           ((uint64_t) buffer[4] << 32u) | ((uint64_t) buffer[5] << 40u) |
           ((uint64_t) buffer[6] << 48u) | ((uint64_t) buffer[7] << 56u);
}

static inline uint64_t rotl(uint64_t x, size_t b)
{
    return (x << b) | (x >> (64 - b));
}

static inline void sipround(uint64_t* v)
{
    v[0] += v[1];
    v[1] = rotl(v[1], 13);
    v[1] ^= v[0];
    v[0] = rotl(v[0], 32);
    v[2] += v[3];
    v[3] = rotl(v[3], 16);
    v[3] ^= v[2];
    v[0] += v[3];
    v[3] = rotl(v[3], 21);
    v[3] ^= v[0];
    v[2] += v[1];
    v[1] = rotl(v[1], 17);
    v[1] ^= v[2];
    v[2] = rotl(v[2], 32);
}

uint64_t
jaeger_siphash(const uint8_t* buffer, size_t size, const uint8_t seed[16])
{
    uint64_t v[4] = {0x736f6d6570736575ULL,
                     0x646f72616e646f6dULL,
                     0x6c7967656e657261ULL,
                     0x7465646279746573ULL};
    const uint64_t k0 = unpack64(&seed[0]);
    const uint64_t k1 = unpack64(&seed[8]);
    const uint8_t* end = buffer + size - (size % sizeof(uint64_t));
    const size_t num_left = size & 7u;
    uint64_t b = ((uint64_t) size) << 56u;
    v[3] ^= k1;
    v[2] ^= k0;
    v[1] ^= k1;
    v[0] ^= k0;

    const uint8_t* iter = buffer;
    for (; iter != end; iter += sizeof(uint64_t)) {
        const uint64_t m = unpack64(iter);
        v[3] ^= m;

        for (int i = 0; i < C_ROUNDS; ++i) {
            sipround(v);
        }

        v[0] ^= m;
    }

    switch (num_left) {
    case 7:
        b |= ((uint64_t) iter[6]) << 48u; /* FALLTHRU */
    case 6:
        b |= ((uint64_t) iter[5]) << 40u; /* FALLTHRU */
    case 5:
        b |= ((uint64_t) iter[4]) << 32u; /* FALLTHRU */
    case 4:
        b |= ((uint64_t) iter[3]) << 24u; /* FALLTHRU */
    case 3:
        b |= ((uint64_t) iter[2]) << 16u; /* FALLTHRU */
    case 2:
        b |= ((uint64_t) iter[1]) << 8u; /* FALLTHRU */
    case 1:
        b |= ((uint64_t) iter[0]);
        break;
    case 0:
        break;
    }

    v[3] ^= b;

    for (int i = 0; i < C_ROUNDS; i++) {
        sipround(v);
    }

    v[0] ^= b;
    v[2] ^= 0xffu;

    for (int i = 0; i < D_ROUNDS; i++) {
        sipround(v);
    }

    return v[0] ^ v[1] ^ v[2] ^ v[3];
}
