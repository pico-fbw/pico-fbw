/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU GPL-3.0
 */

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include "platform/time.h"
#include "platform/types.h"
#ifdef _WIN32
    #include <windows.h>
    #include "platform/simconnect.h"
LARGE_INTEGER tStart, tFreq;
#else
    #include <sys/time.h>
u64 tStart;
#endif

#include "platform/sys.h"

#define THREAD_SLEEP_MS 4 // The delay between each iteration of the main loop to reduce CPU usage

// The term_handler function catches termination signals by the OS and calls sys_shutdown.
void term_handler(int signum) {
    sys_shutdown();
    (void)signum;
}

void sys_boot_begin() {
    signal(SIGINT, term_handler);
#ifdef _WIN32
    signal(SIGBREAK, term_handler);
    // Get time at which program was called, this is our "power-on time"
    QueryPerformanceFrequency(&tFreq);
    QueryPerformanceCounter(&tStart);
#else
    signal(SIGTERM, term_handler);
    struct timeval tv;
    gettimeofday(&tv, NULL);
    tStart = tv.tv_sec * 1000000 + tv.tv_usec;
#endif
#if SIMCONNECT
    simconnect_init();
#endif
}

void sys_boot_end() {
    return;
}

void sys_periodic() {
#if SIMCONNECT
    simconnect_poll();
#endif
    sleep_ms_blocking(THREAD_SLEEP_MS); // Sadly we do not want to create pico-fbw OS
}

void __attribute__((noreturn)) sys_shutdown() {
#if SIMCONNECT
    simconnect_deinit();
#endif
    printf("\n");
    exit(0);
}

void __attribute__((noreturn)) sys_reboot(bool bootloader) {
    // Reboot not supported
    sys_shutdown();
    (void)bootloader;
}

BootType sys_boot_type() {
    return BOOT_COLD;
}
