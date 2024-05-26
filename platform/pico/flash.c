/* Copyright (C) 1883 Thomas Edison - All Rights Reserved
 * You may use, distribute and modify this code under the
 * terms of the BSD 3 clause license, which unfortunately
 * won't be written for another century.
 *
 * A little flash file system for the Raspberry Pico
 *
 * This file utilizes code under the BSD-3-Clause License. See "LICENSE" for details.
 */

/**
 * Thanks to `lurk101` for the implementation of littlefs for the Pico,
 * check it out at https://github.com/lurk101/littlefs-lib/
 */

/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
 */

#include <assert.h>
#include <limits.h>
#include <string.h>
#include "hardware/flash.h"
#include "hardware/regs/addressmap.h"
#include "hardware/sync.h"

#include "platform/types.h"

#include "platform/flash.h"

// Global symbols defined in resources/lfs.S
extern const char __lfs_start[];
extern const char __lfs_end[];

// The filesystem size and location are defined in the linker script, resources/memmap.ld
#define WWWFS_BASE (__lfs_start - XIP_BASE)  // File system offset in system memory space
#define WWWFS_SIZE (__lfs_end - __lfs_start) // File system size
#define LFS_BASE (__lfs_end - XIP_BASE + FLASH_SECTOR_SIZE)
#define LFS_SIZE (PICO_FLASH_SIZE_BYTES - (uintptr_t)LFS_BASE)

static int flash_read(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, void *buffer, lfs_size_t size) {
    assert(block < c->block_count);
    assert(off + size <= c->block_size);
    // Find addres in flash to read from and copy into buffer
    memcpy(buffer, c->context + XIP_NOCACHE_NOALLOC_BASE + (block * c->block_size) + off, size);
    return LFS_ERR_OK;
}

static int flash_prog(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, const void *buffer, lfs_size_t size) {
    assert(block < c->block_count);
    u32 sector = (u32)c->context + (block * c->block_size) + off;
    // Interrupt save and restore is necessary to prevent corruption of flash
    u32 ints = save_and_disable_interrupts();
    flash_range_program(sector, buffer, size);
    restore_interrupts(ints);
    return LFS_ERR_OK;
}

static int flash_erase(const struct lfs_config *c, lfs_block_t block) {
    assert(block < c->block_count);
    u32 sector = (u32)c->context + block * c->block_size;
    u32 ints = save_and_disable_interrupts();
    flash_range_erase(sector, c->block_size);
    restore_interrupts(ints);
    return LFS_ERR_OK;
}

static int flash_sync(const struct lfs_config *c) {
    // Cache is automatically flushed in flash_range_program() so nothing to do here
    return LFS_ERR_OK;
    (void)c; // Supress unused parameter warning
}

bool flash_setup() {
#if PLATFORM_SUPPORTS_WIFI
    wwwfs_cfg.block_count = ((u32)WWWFS_SIZE / FLASH_SECTOR_SIZE);
#endif
    lfs_cfg.block_count = ((u32)LFS_SIZE / FLASH_SECTOR_SIZE);
    return true;
}

lfs_t lfs;
struct lfs_config lfs_cfg = {
    .read = flash_read,
    .prog = flash_prog,
    .erase = flash_erase,
    .sync = flash_sync,
    .context = (void *)LFS_BASE,
    .read_size = 1,
    .prog_size = FLASH_PAGE_SIZE,
    .block_size = FLASH_SECTOR_SIZE,
    .cache_size = (FLASH_SECTOR_SIZE / 4),
    .lookahead_size = 32,
    .block_cycles = 500,
};

#if PLATFORM_SUPPORTS_WIFI
lfs_t wwwfs;
struct lfs_config wwwfs_cfg = {
    .read = flash_read,
    .prog = flash_prog,
    .erase = flash_erase,
    .sync = flash_sync,
    .context = (void *)WWWFS_BASE, // FS_BASE is provided as the context so that block operations know where to read/write
    .read_size = 1,
    .prog_size = FLASH_PAGE_SIZE,    // Minimum write size (256 bytes)
    .block_size = FLASH_SECTOR_SIZE, // Block size must be a multiple of the sector size (4096 bytes)
    // block_count is set in flash_setup()
    .cache_size = (FLASH_SECTOR_SIZE / 4), // Must be a multiple of the block size (1024 bytes)
    .lookahead_size = 32,
    .block_cycles = 500,
};
#endif
