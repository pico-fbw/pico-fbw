/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the MIT License
 */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#ifdef _WIN32
    #include <conio.h>
    #include <windows.h>
#else
    #include <fcntl.h>
#endif

#include "platform/stdio.h"

#define STDIN_BUF_SIZE 512
static char buf[STDIN_BUF_SIZE];

void stdio_setup() {
    // Force stdout/stderr to flush immediately
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);
#ifdef _WIN32
    // To be able to use ANSI escape codes, we need to enable virtual terminal processing
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE) {
        return;
    }
    DWORD dwMode = 0;
    if (!GetConsoleMode(hOut, &dwMode)) {
        return;
    }
    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    if (!SetConsoleMode(hOut, dwMode)) {
        return;
    }
#else
    // Set stdin to be non-blocking
    fcntl(STDIN_FILENO, F_SETFL, fcntl(STDIN_FILENO, F_GETFL) | O_NONBLOCK);
#endif
}

char *stdin_read() {
#ifdef _WIN32
    if (!_kbhit()) {
        return NULL; // No input available
    }

    static int buf_len = 0;
    // Read one character at a time so we never block
    int c = _getche();
    if (c == '\r' || c == '\n') {
        // Enter was pressed, return the line
        printf("\n"); // Move to next line since _getche doesn't on \r
        buf[buf_len] = '\0';
        buf_len = 0;
        if (buf[0] == '\0') {
            return NULL;
        }
        return strdup(buf);
    } else if (c == '\b' && buf_len > 0) {
        // Backspace support
        printf(" \b");
        buf_len--;
    } else if (buf_len < STDIN_BUF_SIZE - 1) {
        buf[buf_len++] = (char)c;
    }

    return NULL; // Line not complete yet
#else
    ssize_t len = read(STDIN_FILENO, buf, sizeof(buf) - 1);
    if (len <= 0) {
        return NULL;
    }
    buf[len] = '\0';
    char *line = strdup(buf);
    if (!line) {
        return NULL;
    }
    line[strcspn(line, "\n")] = '\0'; // Remove trailing newline
    return line;
#endif
}
