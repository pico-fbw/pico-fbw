#ifndef pidtune_h
#define pidtune_h

/**
 * Some example PID auto tuning rules:
 * { {  44, 24,   0 } },  // ZIEGLER_NICHOLS_PI
 * { {  34, 40, 160 } },  // ZIEGLER_NICHOLS_PID
 * { {  64,  9,   0 } },  // TYREUS_LUYBEN_PI
 * { {  44,  9, 126 } },  // TYREUS_LUYBEN_PID
 * { {  66, 80,   0 } },  // CIANCONE_MARLIN_PI
 * { {  66, 88, 162 } },  // CIANCONE_MARLIN_PID
 * { {  28, 50, 133 } },  // PESSEN_INTEGRAL_PID
 * { {  60, 40,  60 } },  // SOME_OVERSHOOT_PID
 * { { 100, 40,  60 } }   // NO_OVERSHOOT_PID
*/

#endif // pidtune_h
