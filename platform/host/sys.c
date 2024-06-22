/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
 */

#include <stdio.h>
#include <stdlib.h>
#include "platform/types.h"
#if defined(_WIN32)
    #include <windows.h>
LARGE_INTEGER tStart, tFreq;
#elif defined(__APPLE__) || defined(__linux__)
    #include <signal.h>
    #include <sys/time.h>
u64 tStart;
#endif

#include "platform/sys.h"

// The term_handler function catches termination signals by the OS and calls sys_shutdown.
#if defined(_WIN32)
BOOL WINAPI term_handler(DWORD dwCtrlType) {
    sys_shutdown();
    return TRUE;
    (void)dwCtrlType;
}
#elif defined(__APPLE__) || defined(__linux__)
void term_handler(int signum) {
    sys_shutdown();
    (void)signum;
}
#endif

void sys_boot_begin() {
#if defined(_WIN32)
    QueryPerformanceFrequency(&tFreq);
    QueryPerformanceCounter(&tStart);
    SetConsoleCtrlHandler(term_handler, TRUE);
#elif defined(__APPLE__) || defined(__linux__)
    // Get time at which program was called, this is our "power-on time"
    struct timeval tv;
    gettimeofday(&tv, NULL);
    tStart = tv.tv_sec * 1000000 + tv.tv_usec;
    signal(SIGINT, term_handler);
#endif
}

void sys_boot_end() {
    return;
}

void sys_periodic() {
    return;
}

void __attribute__((noreturn)) sys_shutdown() {
    printf("\n");
    exit(0);
}

void __attribute__((noreturn)) sys_reboot(bool bootloader) {
    printf("\npico-fbw is running in host mode. Rebooting is not supported, terminating instead.\n");
    exit(0);
    (void)bootloader;
}

BootType sys_boot_type() {
    return BOOT_COLD;
}
