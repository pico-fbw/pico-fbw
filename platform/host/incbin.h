/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU GPL-3.0
 */

/**
 * Embed a binary file into the executable.
 * @param sym symbol to use for the binary data
 * @param filename path to the file to embed
 */
#if defined(__APPLE__) || defined(__MACH__)
    #define INCBIN(sym, filename)                                                                                      \
        __asm__(".section __TEXT,__const\n"                                                                            \
                ".global __" #sym "_start\n__" #sym "_start:\n"                                                        \
                ".incbin \"" filename "\"\n"                                                                           \
                ".global __" #sym "_end\n__" #sym "_end:\n"                                                            \
                ".balign 16\n");                                                                                       \
        extern const unsigned char _##sym##_start[];                                                                   \
        extern const unsigned char _##sym##_end[];
    #define wwwfs_bin_start _wwwfs_bin_start
    #define wwwfs_bin_end _wwwfs_bin_end
#elif defined(__GNUC__)
    #define INCBIN(sym, filename)                                                                                      \
        __asm__(".section .rodata\n"                                                                                   \
                ".global " #sym "_start\n" #sym "_start:\n"                                                            \
                ".incbin \"" filename "\"\n"                                                                           \
                ".global " #sym "_end\n" #sym "_end:\n"                                                                \
                ".balign 16");                                                                                         \
        extern const unsigned char sym##_start[];                                                                      \
        extern const unsigned char sym##_end[];
#else
    #warning "INCBIN is not supported for this compiler"
    #define INCBIN(sym, filename)
#endif
