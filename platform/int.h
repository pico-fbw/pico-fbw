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

#ifndef count_of
    // Returns the number of elements in an array.
    #define count_of(a) (sizeof(a) / sizeof((a)[0]))
#endif

#ifndef clamp
    // Clamps a value `i` between `min` and `max`.
    #define clamp(i, min, max) ((i) < (min) ? (min) : ((i) > (max) ? (max) : (i)))
#endif

// Clamps a value `f` between `min` and `max`.
static inline float clampf(float f, float min, float max) {
    const float t = f < min ? min : f;
    return t > max ? max : t;
}

#ifndef map
    // Maps a value from one range to another.
    #define map(x, in_min, in_max, out_min, out_max) (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min
#endif

// Maps a value from one range to another.
static inline float mapf(float x, float in_min, float in_max, float out_min, float out_max) {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

#ifndef radians
    // Converts degrees to radians.
    #define radians(deg) ((deg) * M_PI / 180.0)
#endif

#ifndef lerp
    // Linearly interpolates between `a` and `b` by `t`.
    #define lerp(a, b, t) (a + t * (b - a))
#endif
