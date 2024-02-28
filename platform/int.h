#pragma once

#include <math.h>
#include <stddef.h>
#include <stdint.h>

typedef uint8_t u8;
typedef uint8_t byte;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

#define count_of(a) (sizeof(a) / sizeof((a)[0]))

// Clamps a value `f` between `min` and `max`.
inline float clampf(float f, float min, float max) {
    const float t = f < min ? min : f;
    return t > max ? max : t;
}

// Clamps a value `i` between `min` and `max`.
inline int clamp(int i, int min, int max) {
    const int t = i < min ? min : i;
    return t > max ? max : t;
}

// Linearly interpolates between `a` and `b` by `t`.
#define lerp(a, b, t) (a + t * (b - a))
