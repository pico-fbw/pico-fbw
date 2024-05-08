#pragma once

// The purpose of sys_shared.h is to allow tStart (and tFreq) to be shared between sys.c and time.c
// This is because tStart needs to be set in sys_boot_begin() in sys.c, but used in time_us() in time.c

#if defined(_WIN32)
    #include <windows.h>
extern LARGE_INTEGER tStart, tFreq;
#elif defined(__APPLE__) || defined(__linux__)
    #include "platform/types.h"
extern u64 tStart;
#endif
