/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
 */

#include <string.h>
#include "platform/flash.h"

#include "lib/semver.h"

#include "sys/print.h"

#include "version.h"

#define FILE_VERSION "version.txt"

VersionCheck version_check(char vstr[]) {
    char version[64];
    if (!vstr)
        return VERSION_ERROR;
    if (vstr[0] == '\0') {
        // No version given, grab from flash
        lfs_file_t file;
        if (lfs_file_open(&lfs, &file, FILE_VERSION, LFS_O_RDONLY | LFS_O_CREAT) != LFS_ERR_OK)
            return VERSION_ERROR;
        i32 read = lfs_file_read(&lfs, &file, version, 32);
        if (lfs_file_close(&lfs, &file) != LFS_ERR_OK)
            return VERSION_ERROR;
        if (read <= 0) {
            printpre("version", "no version found in flash");
            strcpy(vstr, "0.0.0");
            return VERSION_NONE;
        }
        strcpy(vstr, version);
    } else {
        // Version given for us to check against
        if (strlen(vstr) > 63)
            return VERSION_ERROR;
        strcpy(version, vstr);
    }
    // Parse the version strings into the semantic versioning standard, and compare
    semver_t flash;
    if (semver_parse(version, &flash) < 0) {
        semver_free(&flash);
        printpre("version", "unable to parse input version string");
        return VERSION_ERROR;
    }
    semver_t binary;
    if (semver_parse(PICO_FBW_VERSION, &binary) < 0) {
        semver_free(&flash);
        semver_free(&binary);
        printpre("version", "unable to parse binary version string");
        return VERSION_ERROR;
    }
    switch (semver_compare(binary, flash)) {
        case 0:
            return VERSION_SAME; // Equal
        case 1:
            // In most cases this will be a prerelease version but it could also be a user upgrading to a release from
            // prerelease so check that case
            if (binary.prerelease[0] == '\0') {
                return VERSION_OLDER; // Lower
            } else {
                printpre("version", "thanks for testing %s :)", binary.prerelease); // Is this a beta tester spots reference??
                return VERSION_NEWER;                                               // Higher
            }
        case -1:
            return VERSION_OLDER;
    }
    return VERSION_ERROR;
}

bool version_save() {
    lfs_file_t file;
    if (lfs_file_open(&lfs, &file, FILE_VERSION, LFS_O_WRONLY | LFS_O_CREAT) != LFS_ERR_OK)
        return false;
    if (lfs_file_write(&lfs, &file, PICO_FBW_VERSION, strlen(PICO_FBW_VERSION) + 1) != strlen(PICO_FBW_VERSION) + 1) {
        lfs_file_close(&lfs, &file);
        return false;
    }
    if (lfs_file_close(&lfs, &file) != LFS_ERR_OK)
        return false;
    return true;
}
