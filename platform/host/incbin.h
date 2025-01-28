/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU GPL-3.0
 */

/**
 * Embed a binary file into the executable.
 * @param sym symbol to use for the binary data
 * @param filename path to the file to embed
 */
#define INCBIN(sym, filename)                                                                                          \
    __asm__(".section .data\n"                                                                                         \
            ".global " #sym "_start\n" #sym "_start:\n"                                                                \
            ".incbin \"" filename "\"\n"                                                                               \
            ".global " #sym "_end\n" #sym "_end:\n"                                                                    \
            ".balign 16");                                                                                             \
    extern const unsigned char sym##_start[];                                                                          \
    extern const unsigned char sym##_end[];
