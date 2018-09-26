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

/**
 * @file
 * @internal
 * String utilities.
 */

#ifndef JAEGERTRACINGC_ENDIAN_H
#define JAEGERTRACINGC_ENDIAN_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32
#define BYTESWAP64(x) _byteswap_uint64((x))
#define BYTESWAP32(x) _byteswap_ulong((x))
#elif defined(__APPLE__)
#include <libkern/OSByteOrder.h>
#define BYTESWAP64(x) OSSwapConstInt64((x))
#define BYTESWAP32(x) OSSwapConstInt32((x))
#else
#include <endian.h>
#define BYTESWAP64(x) be64toh((x))
#define BYTESWAP32(x) be32toh((x))
#endif /* _WIN32 */

#ifdef JAEGERTRACINGC_LITTLE_ENDIAN
#define BIG_ENDIAN_64_TO_HOST(x) BYTESWAP64(x)
#define BIG_ENDIAN_32_TO_HOST(x) BYTESWAP32(x)
#else
#define BIG_ENDIAN_64_TO_HOST(x) (x)
#define BIG_ENDIAN_32_TO_HOST(x) (x)
#endif /* JAEGERTRACINGC_LITTLE_ENDIAN */

#define HOST_TO_BIG_ENDIAN_64(x) BIG_ENDIAN_64_TO_HOST(x)
#define HOST_TO_BIG_ENDIAN_32(x) BIG_ENDIAN_32_TO_HOST(x)

#ifdef __cplusplus
}
#endif

#endif /* JAEGERTRACINGC_ENDIAN_H */
