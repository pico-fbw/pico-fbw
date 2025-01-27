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
    #define mkdir(path, mode) _mkdir(path) // Compatibility with *nix mkdir
#else
    #include <sys/stat.h>
    #include <sys/types.h>
    #define SEP "/"
#endif

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

// To emulate flash memory on a microcontroller, we use files on the host system.
// The file (BINNAME) is stored inside a directory (BINDIR)
// in the user's home directory (*nix) or AppData directory (Windows).
#define BINDIR ".pico-fbw"
#define LFS_BINNAME "lfs.bin"
#define WWWFS_BINNAME "wwwfs.bin"
// Will store the full path to its respective file; set in flash_setup()
char *lfsFilepath;
char *wwwfsFilepath;

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

static int flash_read(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, void *buffer, lfs_size_t size) {
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

static int flash_prog(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, const void *buffer,
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

static int flash_erase(const struct lfs_config *c, lfs_block_t block) {
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

static int flash_sync(const struct lfs_config *c) {
    // No need for sync, host kernel will take care of it
    return LFS_ERR_OK;
    (void)c;
}

/**
 * Makes a path to the directory where filesystem binary files should be stored.
 * @param binname the name of the binary file to make a path to, or NULL to make a path to the directory
 * @param path the output path
 * @return true if successful
 */
static bool make_path(const char *binname, char **path) {
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
    const char *env = "";
#endif
    if (!binname) {
        *path = (char *)malloc(strlen(env) + strlen(SEP) + strlen(BINDIR) + 1);
        if (!*path) {
            return false;
        }
        sprintf(*path, "%s%s%s", env, SEP, BINDIR);
        return true;
    }
    *path = (char *)malloc(strlen(env) + strlen(SEP) * 2 + strlen(BINDIR) + strlen(binname) + 1);
    if (!*path) {
        return false;
    }
    sprintf(*path, "%s%s%s%s%s", env, SEP, BINDIR, SEP, binname);
    return true;
}

/**
 * @param path the path to the file
 * @return true if the file exists
 */
static bool file_exists(const char *path) {
    FILE *file = fopen(path, "rb");
    if (!file) {
        return false;
    }
    fclose(file);
    return true;
}

/**
 * Creates a blank binary file of FS_SIZE bytes.
 * @param path the path to the file
 * @return true if successful
 */
static bool create_flash_bin(const char *path) {
    FILE *file = fopen(path, "wb");
    if (!file) {
        return false;
    }
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
    fclose(file);
    return true;
}

bool flash_setup() {
    // Get the path to the directory where the filesystem files are stored
    char *dirpath;
    if (!make_path(NULL, &dirpath)) {
        return false;
    }
    // Create the directory if it doesn't exist
    if (mkdir(dirpath, 0755) != 0 && errno != EEXIST) {
        free(dirpath);
        return false;
    }
    free(dirpath);

    // Containing directory is confirmed to exist, we can now get paths to the filesystem files
    if (!make_path(LFS_BINNAME, &lfsFilepath) || !make_path(WWWFS_BINNAME, &wwwfsFilepath)) {
        return false;
    }
    // Also create LFS_BIN if it doesn't exist
    if (!file_exists(lfsFilepath) && !create_flash_bin(lfsFilepath)) {
        return false;
    }
    // Pass the filepaths in as context to littlefs so block operations can access it
    lfs_cfg.context = (void *)lfsFilepath;
#if PLATFORM_SUPPORTS_WIFI
    // We don't create WWWFS_BIN as cmake should be handling that
    // If it doesn't exist, throw a warning and continue
    if (!file_exists(wwwfsFilepath)) {
        fprintf(stderr, "WARNING: wwwfs.bin not found, web interface will not be available\n");
    }
    wwwfs_cfg.context = (void *)wwwfsFilepath;
#endif
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

#if PLATFORM_SUPPORTS_WIFI
lfs_t wwwfs;
struct lfs_config wwwfs_cfg = {
    .read = flash_read,
    .prog = flash_prog,
    .erase = flash_erase,
    .sync = flash_sync,
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
