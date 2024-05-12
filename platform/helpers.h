#pragma once

#include "platform/types.h"

/* Number operation helpers */

#ifndef clamp
    // Clamps a value `i` between `min` and `max`.
    #define clamp(i, min, max) ((i) < (min) ? (min) : ((i) > (max) ? (max) : (i)))
#endif

// Clamps a value `f` between `min` and `max`.
static inline f32 clampf(f32 f, f32 min, f32 max) {
    const f32 t = f < min ? min : f;
    return t > max ? max : t;
}

#ifndef map
    // Maps a value `x` from the range `in_min` to `in_max` to the range `out_min` to `out_max`.
    #define map(x, in_min, in_max, out_min, out_max) (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min
#endif

// Maps a value `f` from the range `in_min` to `in_max` to the range `out_min` to `out_max`.
static inline f32 mapf(f32 f, f32 in_min, f32 in_max, f32 out_min, f32 out_max) {
    return (f - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

#ifndef radians
    // Converts degrees to radians.
    #define radians(deg) (deg * M_PI / 180.0)
#endif

#ifndef degrees
    // Converts radians to degrees.
    #define degrees(rad) (rad * 180.0 / M_PI)
#endif

#ifndef lerp
    // Linearly interpolates between `a` and `b` by `t`.
    #define lerp(a, b, t) (a + t * (b - a))
#endif

/* Array helpers */

#ifndef count_of
    // Returns the number of elements in an array.
    #define count_of(a) (sizeof(a) / sizeof((a)[0]))
#endif

/* Compiler optimization helpers */

#ifndef likely
    // Hints the compiler that the expression is likely to be true.
    #define likely(x) __builtin_expect(!!(x), 1)
#endif

#ifndef unlikely
    // Hints the compiler that the expression is likely to be false.
    #define unlikely(x) __builtin_expect(!!(x), 0)
#endif

/* Bit manipulation helpers */

#ifndef bit
    // Returns a bit mask with the bit at position `n` set.
    #define bit(n) (1 << (n))
#endif
