/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU GPL-3.0
 */

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
    #include <direct.h>
    #define SEP "\\"
    #define mkdir(path, mode) _mkdir(path) // Compatibility with *nix mkdir()
#else
    #include <sys/stat.h>
    #include <sys/types.h>
    #define SEP "/"
#endif

#include "platform/helpers.h"

#include "platform/flash.h"

// FS configuration, littlefs documentation explains these settings in detail (see lib/lfs.h)
// Since we are reading and writing from files and not directly from flash memory, most of these settings are arbitrary.
#define READ_SIZE 1
#define WRITE_SIZE 1
#define BLOCK_SIZE 4096 // 4 KB blocks because they seem pretty comomon (and I felt like it)
#define CACHE_SIZE 512
#define LOOKAHEAD_SIZE 128
#define BLOCK_CYCLES -1 // Don't need to worry about wear leveling on a host system
#define FS_SIZE 262144  // 256 KB

/* ---  POSIX implementation (lfs) --- */

// To emulate flash memory on a microcontroller, we use a file on the host system.
// The file (BINNAME) is stored inside a directory (BINDIR)
// in the user's home directory (*nix) or AppData directory (Windows).
#define BINDIR ".pico-fbw"
#define BINNAME "lfs.bin"
char *filepath; // Will store the full path to the file, set in flash_setup()

/**
 * Creates the directory path for the littlefs data file.
 * @param path pointer to a char pointer that will be set to the path
 * @return true if successful
 */
static bool create_dirpath(char **path) {
    // BINDIR should be located in the respective os's program data directory
#if defined(_WIN32)
    const char *env = getenv("APPDATA");
    if (!env) {
        return false;
    }
#elif defined(__APPLE__) || defined(__linux__)
    const char *env = getenv("HOME");
    if (!env) {
        return false;
    }
#else
    // Unknown platform, create the file in the current directory
    const char *env = "";
#endif
    // Allocate memory for and build the path
    // Even though we're only returning the path to the directory, space should be allocated for the filename,
    // as it will be appended later
    *path = (char *)malloc(strlen(env) + strlen(SEP) * 2 + strlen(BINDIR) + strlen(BINNAME) + 1);
    if (!*path) {
        return false;
    }
    sprintf(*path, "%s%s%s", env, SEP, BINDIR);
    return true;
}

/**
 * Opens the littlefs data file and seeks to the specified offset.
 * @param c the littlefs configuration pertaining to the current operation
 * @param mode the mode to open the file in (fopen mode string)
 * @param offset the offset to seek to
 * @return the file pointer, or NULL if an error occurred (check errno for details)
 */
static FILE *open_and_seek(const struct lfs_config *c, const char *mode, long offset) {
    FILE *file = fopen((char *)c->context, mode);
    if (!file) {
        return NULL;
    }
    if (fseek(file, offset, SEEK_SET) != 0) {
        fclose(file);
        return NULL;
    }
    return file;
}

static int flash_read_posix(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, void *buffer,
                            lfs_size_t size) {
    assert(block < c->block_count);
    assert(off + size <= c->block_size);
    FILE *file = open_and_seek(c, "rb", block * c->block_size + off);
    if (!file) {
        return LFS_ERR_IO;
    }
    if (fread(buffer, size, 1, file) != 1) {
        fclose(file);
        return LFS_ERR_IO;
    }
    fclose(file);
    return LFS_ERR_OK;
}

static int flash_prog_posix(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, const void *buffer,
                            lfs_size_t size) {
    assert(block < c->block_count);
    FILE *file = open_and_seek(c, "rb+", block * c->block_size + off);
    if (!file) {
        return LFS_ERR_IO;
    }
    if (fwrite(buffer, size, 1, file) != 1) {
        fclose(file);
        return LFS_ERR_IO;
    }
    fclose(file);
    return LFS_ERR_OK;
}

static int flash_erase_posix(const struct lfs_config *c, lfs_block_t block) {
    assert(block < c->block_count);
    FILE *file = open_and_seek(c, "rb+", block * c->block_size);
    if (!file) {
        return LFS_ERR_IO;
    }
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

static int flash_sync_posix(const struct lfs_config *c) {
    // No need for sync, host kernel will take care of it
    return LFS_ERR_OK;
    (void)c;
}

/* --- Binary implementation (wwwfs) --- */

#if PLATFORM_SUPPORTS_WIFI

// Embed wwwfs data into the binary
INCBIN(wwwfs_bin, "lfs.bin")

static int flash_read_bin(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, void *buffer, lfs_size_t size) {
    assert(block < c->block_count);
    assert(off + size <= c->block_size);
    memcpy(buffer, c->context + (block * c->block_size) + off, size);
    return LFS_ERR_OK;
}

static int flash_prog_bin(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, const void *buffer,
                          lfs_size_t size) {
    // Binary cannot write to itself
    return LFS_ERR_IO;
    (void)c;
    (void)block;
    (void)off;
    (void)buffer;
    (void)size;
}

static int flash_erase_bin(const struct lfs_config *c, lfs_block_t block) {
    return LFS_ERR_IO;
    (void)c;
    (void)block;
}

static int flash_sync_bin(const struct lfs_config *c) {
    return LFS_ERR_OK;
    (void)c;
}

#endif // PLATFORM_SUPPORTS_WIFI

/* --- End implementations --- */

bool flash_setup() {
    // Create the path to the directory containing our littlefs emulation file
    if (!create_dirpath(&filepath)) {
        return false;
    }
    // Make the directory if it doesn't exist
    if (mkdir(filepath, 0755) != 0 && errno != EEXIST) {
        free(filepath);
        return false;
    }
    // Directory is now confirmed to exist, add the filename and now we have the full path
    strcat(filepath, SEP);
    strcat(filepath, BINNAME);

    // Confirm the file exists and create it if it doesn't
    FILE *file = fopen(filepath, "rb");
    if (!file) {
        file = fopen(filepath, "wb");
        if (!file) {
            return false;
        }
        // File now created, set its size to the total filesystem size
        // This makes everything else much simpler, and the file will be quite small anyway
        if (fseek(file, FS_SIZE - 1, SEEK_SET) != 0) {
            fclose(file);
            free(filepath);
            return false;
        }
        if (fwrite("\0", 1, 1, file) < 1) {
            fclose(file);
            free(filepath);
            return false;
        }
    }
    fclose(file);
    // Pass the filepath in as context to littlefs so block operations can access it
    lfs_cfg.context = (void *)filepath;
    // Since littlefs needs to access the path, don't free it
    return true;
}

// clang-format off

lfs_t lfs;
struct lfs_config lfs_cfg = {
    .read = flash_read_posix,
    .prog = flash_prog_posix,
    .erase = flash_erase_posix,
    .sync = flash_sync_posix,
    // context is set in flash_setup()
    .read_size = READ_SIZE,
    .prog_size = WRITE_SIZE,
    .block_size = BLOCK_SIZE,
    .block_count = (FS_SIZE / BLOCK_SIZE),
    .cache_size = CACHE_SIZE,
    .lookahead_size = LOOKAHEAD_SIZE,
    .block_cycles = BLOCK_CYCLES,
};

#if PLATFORM_SUPPORTS_WIFI

lfs_t wwwfs;
struct lfs_config wwwfs_cfg = {
    .read = flash_read_bin,
    .prog = flash_prog_bin,
    .erase = flash_erase_bin,
    .sync = flash_sync_bin,
    .context = (void *)_wwwfs_bin_start, // Symbol exported by INCBIN()
    .read_size = READ_SIZE,
    .prog_size = WRITE_SIZE,
    .block_size = BLOCK_SIZE,
    .block_count = (FS_SIZE / BLOCK_SIZE),
    .cache_size = CACHE_SIZE,
    .lookahead_size = LOOKAHEAD_SIZE,
    .block_cycles = BLOCK_CYCLES,
};
#endif

// clang-format on
