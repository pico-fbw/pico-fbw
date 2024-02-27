/*
 * lfs utility functions
 *
 * Copyright (c) 2022, The littlefs authors.
 * Copyright (c) 2017, Arm Limited. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 */
#pragma once

// Users can override lfs_util.h with their own configuration by defining
// LFS_CONFIG as a header file to include (-DLFS_CONFIG=lfs_config.h).
//
// If LFS_CONFIG is used, none of the default utils will be emitted and must be
// provided by the config file. To start, I would suggest copying lfs_util.h
// and modifying as needed.
#ifdef LFS_CONFIG
#define LFS_STRINGIZE(x) LFS_STRINGIZE2(x)
#define LFS_STRINGIZE2(x) #x
#include LFS_STRINGIZE(LFS_CONFIG)
#else

// System includes
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <inttypes.h>
#include "platform/int.h"

#ifndef LFS_NO_MALLOC
#include <stdlib.h>
#endif
#ifndef LFS_NO_ASSERT
#include <assert.h>
#endif
#if !defined(LFS_NO_DEBUG) || \
        !defined(LFS_NO_WARN) || \
        !defined(LFS_NO_ERROR) || \
        defined(LFS_YES_TRACE)
#include <stdio.h>
#endif

#ifdef __cplusplus
extern "C"
{
#endif


// Macros, may be replaced by system specific wrappers. Arguments to these
// macros must not have side-effects as the macros can be removed for a smaller
// code footpri32

// Logging functions
#ifndef LFS_TRACE
#ifdef LFS_YES_TRACE
#define LFS_TRACE_(fmt, ...) \
    printf("%s:%d:trace: " fmt "%s\n", __FILE__, __LINE__, __VA_ARGS__)
#define LFS_TRACE(...) LFS_TRACE_(__VA_ARGS__, "")
#else
#define LFS_TRACE(...)
#endif
#endif

#ifndef LFS_DEBUG
#ifndef LFS_NO_DEBUG
#define LFS_DEBUG_(fmt, ...) \
    printf("%s:%d:debug: " fmt "%s\n", __FILE__, __LINE__, __VA_ARGS__)
#define LFS_DEBUG(...) LFS_DEBUG_(__VA_ARGS__, "")
#else
#define LFS_DEBUG(...)
#endif
#endif

#ifndef LFS_WARN
#ifndef LFS_NO_WARN
#define LFS_WARN_(fmt, ...) \
    printf("%s:%d:warn: " fmt "%s\n", __FILE__, __LINE__, __VA_ARGS__)
#define LFS_WARN(...) LFS_WARN_(__VA_ARGS__, "")
#else
#define LFS_WARN(...)
#endif
#endif

#ifndef LFS_ERROR
#ifndef LFS_NO_ERROR
#define LFS_ERROR_(fmt, ...) \
    printf("%s:%d:error: " fmt "%s\n", __FILE__, __LINE__, __VA_ARGS__)
#define LFS_ERROR(...) LFS_ERROR_(__VA_ARGS__, "")
#else
#define LFS_ERROR(...)
#endif
#endif

// Runtime assertions
#ifndef LFS_ASSERT
#ifndef LFS_NO_ASSERT
#define LFS_ASSERT(test) assert(test)
#else
#define LFS_ASSERT(test)
#endif
#endif


// Builtin functions, these may be replaced by more efficient
// toolchain-specific implementations. LFS_NO_i32RINSICS falls back to a more
// expensive basic C implementation for debugging purposes

// Min/max functions for unsigned 32-bit numbers
static inline u32 lfs_max(u32 a, u32 b) {
    return (a > b) ? a : b;
}

static inline u32 lfs_min(u32 a, u32 b) {
    return (a < b) ? a : b;
}

// Align to nearest multiple of a size
static inline u32 lfs_aligndown(u32 a, u32 alignment) {
    return a - (a % alignment);
}

static inline u32 lfs_alignup(u32 a, u32 alignment) {
    return lfs_aligndown(a + alignment-1, alignment);
}

// Find the smallest power of 2 greater than or equal to a
static inline u32 lfs_npw2(u32 a) {
#if !defined(LFS_NO_i32RINSICS) && (defined(__GNUC__) || defined(__CC_ARM))
    return 32 - __builtin_clz(a-1);
#else
    u32 r = 0;
    u32 s;
    a -= 1;
    s = (a > 0xffff) << 4; a >>= s; r |= s;
    s = (a > 0xff  ) << 3; a >>= s; r |= s;
    s = (a > 0xf   ) << 2; a >>= s; r |= s;
    s = (a > 0x3   ) << 1; a >>= s; r |= s;
    return (r | (a >> 1)) + 1;
#endif
}

// Count the number of trailing binary zeros in a
// lfs_ctz(0) may be undefined
static inline u32 lfs_ctz(u32 a) {
#if !defined(LFS_NO_i32RINSICS) && defined(__GNUC__)
    return __builtin_ctz(a);
#else
    return lfs_npw2((a & -a) + 1) - 1;
#endif
}

// Count the number of binary ones in a
static inline u32 lfs_popc(u32 a) {
#if !defined(LFS_NO_i32RINSICS) && (defined(__GNUC__) || defined(__CC_ARM))
    return __builtin_popcount(a);
#else
    a = a - ((a >> 1) & 0x55555555);
    a = (a & 0x33333333) + ((a >> 2) & 0x33333333);
    return (((a + (a >> 4)) & 0xf0f0f0f) * 0x1010101) >> 24;
#endif
}

// Find the sequence comparison of a and b, this is the distance
// between a and b ignoring overflow
static inline i32 lfs_scmp(u32 a, u32 b) {
    return (i32)(unsigned)(a - b);
}

// Convert between 32-bit little-endian and native order
static inline u32 lfs_fromle32(u32 a) {
#if (defined(  BYTE_ORDER  ) && defined(  ORDER_LITTLE_ENDIAN  ) &&   BYTE_ORDER   ==   ORDER_LITTLE_ENDIAN  ) || \
    (defined(__BYTE_ORDER  ) && defined(__ORDER_LITTLE_ENDIAN  ) && __BYTE_ORDER   == __ORDER_LITTLE_ENDIAN  ) || \
    (defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
    return a;
#elif !defined(LFS_NO_i32RINSICS) && ( \
    (defined(  BYTE_ORDER  ) && defined(  ORDER_BIG_ENDIAN  ) &&   BYTE_ORDER   ==   ORDER_BIG_ENDIAN  ) || \
    (defined(__BYTE_ORDER  ) && defined(__ORDER_BIG_ENDIAN  ) && __BYTE_ORDER   == __ORDER_BIG_ENDIAN  ) || \
    (defined(__BYTE_ORDER__) && defined(__ORDER_BIG_ENDIAN__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__))
    return __builtin_bswap32(a);
#else
    return (((u8*)&a)[0] <<  0) |
           (((u8*)&a)[1] <<  8) |
           (((u8*)&a)[2] << 16) |
           (((u8*)&a)[3] << 24);
#endif
}

static inline u32 lfs_tole32(u32 a) {
    return lfs_fromle32(a);
}

// Convert between 32-bit big-endian and native order
static inline u32 lfs_frombe32(u32 a) {
#if !defined(LFS_NO_i32RINSICS) && ( \
    (defined(  BYTE_ORDER  ) && defined(  ORDER_LITTLE_ENDIAN  ) &&   BYTE_ORDER   ==   ORDER_LITTLE_ENDIAN  ) || \
    (defined(__BYTE_ORDER  ) && defined(__ORDER_LITTLE_ENDIAN  ) && __BYTE_ORDER   == __ORDER_LITTLE_ENDIAN  ) || \
    (defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__))
    return __builtin_bswap32(a);
#elif (defined(  BYTE_ORDER  ) && defined(  ORDER_BIG_ENDIAN  ) &&   BYTE_ORDER   ==   ORDER_BIG_ENDIAN  ) || \
    (defined(__BYTE_ORDER  ) && defined(__ORDER_BIG_ENDIAN  ) && __BYTE_ORDER   == __ORDER_BIG_ENDIAN  ) || \
    (defined(__BYTE_ORDER__) && defined(__ORDER_BIG_ENDIAN__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
    return a;
#else
    return (((u8*)&a)[0] << 24) |
           (((u8*)&a)[1] << 16) |
           (((u8*)&a)[2] <<  8) |
           (((u8*)&a)[3] <<  0);
#endif
}

static inline u32 lfs_tobe32(u32 a) {
    return lfs_frombe32(a);
}

// Calculate CRC-32 with polynomial = 0x04c11db7
#ifdef LFS_CRC
u32 lfs_crc(u32 crc, const void *buffer, size_t size) {
    return LFS_CRC(crc, buffer, size)
}
#else
u32 lfs_crc(u32 crc, const void *buffer, size_t size);
#endif

// Allocate memory, only used if buffers are not provided to littlefs
//
// littlefs current has no alignment requirements, as it only allocates
// byte-level buffers.
static inline void *lfs_malloc(size_t size) {
#if defined(LFS_MALLOC)
    return LFS_MALLOC(size);
#elif !defined(LFS_NO_MALLOC)
    return malloc(size);
#else
    (void)size;
    return NULL;
#endif
}

// Deallocate memory, only used if buffers are not provided to littlefs
static inline void lfs_free(void *p) {
#if defined(LFS_FREE)
    LFS_FREE(p);
#elif !defined(LFS_NO_MALLOC)
    free(p);
#else
    (void)p;
#endif
}


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
