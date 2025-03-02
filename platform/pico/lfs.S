/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the MIT License
 */

#include "pico/asm_helper.S"

# lfs.bin will be generated (when necessary) using mklittlefs during the build.
# The purpose of this file is to include the generated lfs.bin file into the compiled binary.

pico_default_asm_setup
# .lfs is a section (defined in memmap.ld) that has been created to store lfs.bin.
# It's exactly 256KB in size, same as lfs.bin.
.section .lfs , "a" , %progbits
.global __lfs_start
.global __lfs_end
__lfs_start:
# can be found at <build>/generated/www/lfs.bin
.incbin "lfs.bin"
__lfs_end:
