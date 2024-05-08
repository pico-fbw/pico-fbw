/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
 */

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if defined(_WIN32)
    #include <direct.h>
    #define SEP "\\"
    #define mkdir(path, mode) _mkdir(path) // Compatibility with *nix mkdir
#elif defined(__APPLE__) || defined(__linux__)
    #include <sys/stat.h>
    #include <sys/types.h>
    #define SEP "/"
#endif

#include "platform/flash.h"

// FS configuration, littlefs documentation explains these settings in detail (see lib/lfs.h)
// Since we are reading and writing from files and not directly from flash memory, most of these settings are arbitrary.
#define READ_SIZE 1
#define WRITE_SIZE 1
#define BLOCK_SIZE 1024 // 1 KB blocks because I felt like it
#define CACHE_SIZE 512
#define LOOKAHEAD_SIZE 128
#define BLOCK_CYCLES -1 // Don't need to worry about wear leveling on a host system
#define FS_SIZE 262144  // 256 KB

// To emulate flash memory on a microcontroller, we use a file on the host system.
// The file (BINNAME) is stored inside a directory (BINDIR) in the user's home directory (*nix) or AppData directory (Windows).
#define BINDIR ".pico-fbw"
#define BINNAME "lfs.bin"
char *filepath; // Will store the full path to the file, set in flash_setup()

/**
 * Opens the littlefs data file and seeks to the specified offset.
 * @param c the littlefs configuration pertaining to the current operation
 * @param mode the mode to open the file in (fopen mode string)
 * @param offset the offset to seek to
 * @return the file pointer, or NULL if an error occurred (check errno for details)
 */
static FILE *open_and_seek(const struct lfs_config *c, const char *mode, long offset) {
    FILE *file = fopen((char *)c->context, mode);
    if (!file)
        return NULL;
    if (fseek(file, offset, SEEK_SET) != 0) {
        fclose(file);
        return NULL;
    }
    return file;
}

static int flash_read(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, void *buffer, lfs_size_t size) {
    assert(block < c->block_count);
    assert(off + size <= c->block_size);
    FILE *file = open_and_seek(c, "rb", block * c->block_size + off);
    if (!file)
        return LFS_ERR_IO;
    if (fread(buffer, size, 1, file) != 1) {
        fclose(file);
        return LFS_ERR_IO;
    }
    fclose(file);
    return LFS_ERR_OK;
}

static int flash_prog(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, const void *buffer, lfs_size_t size) {
    assert(block < c->block_count);
    FILE *file = open_and_seek(c, "rb+", block * c->block_size + off);
    if (!file)
        return LFS_ERR_IO;
    if (fwrite(buffer, size, 1, file) != 1) {
        fclose(file);
        return LFS_ERR_IO;
    }
    fclose(file);
    return LFS_ERR_OK;
}

static int flash_erase(const struct lfs_config *c, lfs_block_t block) {
    assert(block < c->block_count);
    FILE *file = open_and_seek(c, "rb+", block * c->block_size);
    if (!file)
        return LFS_ERR_IO;
    for (lfs_size_t i = 0; i < c->block_size; i++) {
        // On real flash memory, erasing a block sets all bits to 1 (0xFF)
        if (fputc(0xFF, file) == EOF) {
            fclose(file);
            return LFS_ERR_IO;
        }
    }
    fclose(file);
    return LFS_ERR_OK;
}

static int flash_sync(const struct lfs_config *c) {
    // No need for sync, host kernel will take care of it
    return LFS_ERR_OK;
    (void)c;
}

bool flash_setup() {
    // Determine the filepath and allocate memory for it
#if defined(_WIN32)
    const char *appdata = getenv("APPDATA");
    if (!appdata)
        return false;
    filepath = (char *)malloc(strlen(appdata) + strlen(BINDIR) + strlen(BINNAME) + 2);
    if (!filepath)
        return false;
    sprintf(filepath, "%s%s%s", appdata, SEP, BINDIR);
#elif defined(__APPLE__) || defined(__linux__)
    const char *home = getenv("HOME");
    if (!home)
        return false;
    filepath = (char *)malloc(strlen(home) + strlen(BINDIR) + strlen(BINNAME) + 2);
    if (!filepath)
        return false;
    sprintf(filepath, "%s%s%s", home, SEP, BINDIR);
#else
    // Unknown platform, create the file in the current directory
    filepath = (char *)malloc(strlen(BINNAME));
    if (!filepath)
        return false;
    strcpy(filepath, BINNAME);
#endif

    // Create the directory if it doesn't exist
    if (mkdir(filepath, 0755) != 0 && errno != EEXIST) {
        free(filepath);
        return false;
    }
    // Directory is now confirmed to exist, add the filename and now we have the full path
    strcat(filepath, SEP);
    strcat(filepath, BINNAME);

    // Confirm the file exist and create it if it doesn't
    FILE *file = fopen(filepath, "rb");
    if (!file) {
        file = fopen(filepath, "wb");
        if (!file)
            return false;
        // Set the file size to the total filesystem size
        // This makes everything else much simpler, and the file will be quite small anyway
        if (fseek(file, FS_SIZE - 1, SEEK_SET) != 0) {
            fclose(file);
            return false;
        }
        if (fwrite("\0", 1, 1, file) < 1) {
            fclose(file);
            return false;
        }
    }
    fclose(file);
    // Pass the filepath in as context to littlefs so block operations can access it
    lfs_cfg.context = (void *)filepath;
    return true;
}

// clang-format off

lfs_t lfs;
struct lfs_config lfs_cfg = {
    .read = flash_read,
    .prog = flash_prog,
    .erase = flash_erase,
    .sync = flash_sync,
    // context is set in flash_setup()
    .read_size = READ_SIZE,
    .prog_size = WRITE_SIZE,
    .block_size = BLOCK_SIZE,
    .block_count = (FS_SIZE / BLOCK_SIZE),
    .cache_size = CACHE_SIZE,
    .lookahead_size = LOOKAHEAD_SIZE,
    .block_cycles = BLOCK_CYCLES,
};

// clang-format on
