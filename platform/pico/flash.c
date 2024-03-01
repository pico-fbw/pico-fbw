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
#include "platform/int.h"

#include "hardware/flash.h"
#include "hardware/regs/addressmap.h"
#include "hardware/sync.h"

#include "platform/flash.h"

#define FS_SIZE (256 * 1024)                                            // 256K
static const byte *FS_BASE = (byte *)(PICO_FLASH_SIZE_BYTES - FS_SIZE); // File system offset in system memory space

static int flash_read(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, void *buffer, lfs_size_t size) {
    assert(block < c->block_count);
    assert(off + size <= c->block_size);
    // Find addres in flash to read from and copy into buffer
    memcpy(buffer, FS_BASE + XIP_NOCACHE_NOALLOC_BASE + (block * c->block_size) + off, size);
    return LFS_ERR_OK;
}

static int flash_prog(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, const void *buffer, lfs_size_t size) {
    assert(block < c->block_count);
    u32 sector = (u32)FS_BASE + (block * c->block_size) + off;
    // Interrupt save and restore is necessary to prevent corruption of flash
    u32 ints = save_and_disable_interrupts();
    flash_range_program(sector, buffer, size);
    restore_interrupts(ints);
    return LFS_ERR_OK;
}

static int flash_erase(const struct lfs_config *c, lfs_block_t block) {
    assert(block < c->block_count);
    u32 sector = (u32)FS_BASE + block * c->block_size;
    u32 ints = save_and_disable_interrupts();
    flash_range_erase(sector, c->block_size);
    restore_interrupts(ints);
    return LFS_ERR_OK;
}

static int flash_sync(const struct lfs_config *c) {
    // Cache is automatically flushed in flash_range_program() so nothing to do here
    return LFS_ERR_OK;
    (void)c; // Supress unused parameter warning; the config struct is required by littlefs but not used here
}

struct lfs_config lfs_cfg = {
    .read = flash_read,
    .prog = flash_prog,
    .erase = flash_erase,
    .sync = flash_sync,
    .read_size = 1,
    .prog_size = FLASH_PAGE_SIZE,               // Minimum write size (256 bytes)
    .block_size = FLASH_SECTOR_SIZE,            // Block size must be a multiple of the sector size (4096 bytes)
    .block_count = FS_SIZE / FLASH_SECTOR_SIZE, // Number of blocks in the filesystem (64 blocks make up 256K FS_SIZE)
    .cache_size = FLASH_SECTOR_SIZE / 4,        // Must be a multiple of the block size (1024 bytes)
    .lookahead_size = 32,
    .block_cycles = 500};

lfs_t lfs;
