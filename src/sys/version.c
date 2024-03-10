/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
 */

#include <string.h>
#include "platform/flash.h"

#include "lib/semver.h"

#include "sys/print.h"

#include "version.h"

i32 version_check(char vstr[]) {
    char version[64];
    if (!vstr || vstr[0] == '\0') {
        // No version given, grab from flash
        lfs_file_t file;
        if (lfs_file_open(&lfs, &file, "version", LFS_O_RDONLY | LFS_O_CREAT) != LFS_ERR_OK)
            return -4;
        i32 read = lfs_file_read(&lfs, &file, version, 32);
        if (lfs_file_close(&lfs, &file) != LFS_ERR_OK)
            return -4;
        if (read <= 0) {
            print("[version] no version found in flash");
            strcpy(vstr, "0.0.0");
            return -2;
        }
        strcpy(vstr, version);
    } else {
        // Version given for us to check against
        if (strlen(vstr) > 63)
            return -3;
        strcpy(version, vstr);
    }
    // Parse the version strings into the semantic versioning standard, and compare
    semver_t flash;
    if (semver_parse(version, &flash) < 0) {
        semver_free(&flash);
        print("[version] unable to parse input version string!");
        return -3;
    }
    semver_t binary;
    if (semver_parse(PICO_FBW_VERSION, &binary) < 0) {
        semver_free(&flash);
        semver_free(&binary);
        print("[version] unable to parse binary version string!");
        return -3;
    }
    switch (semver_compare(binary, flash)) {
        case 0:
            return 0; // Equal
        case 1:
            // In most cases this will be a prerelease version but it could also be a user upgrading to a release from
            // prerelease so check that case
            if (binary.prerelease[0] == '\0') {
                return -1; // Lower
            } else {
                print("[version] thanks for testing %s :)", binary.prerelease);
                return 1; // Higher
            }
        case -1:
            return -1;
    }
    return -2;
}

bool version_save() {
    lfs_file_t file;
    if (lfs_file_open(&lfs, &file, "version", LFS_O_WRONLY | LFS_O_CREAT) != LFS_ERR_OK)
        return false;
    if (lfs_file_write(&lfs, &file, PICO_FBW_VERSION, strlen(PICO_FBW_VERSION) + 1) != strlen(PICO_FBW_VERSION) + 1) {
        lfs_file_close(&lfs, &file);
        return false;
    }
    if (lfs_file_close(&lfs, &file) != LFS_ERR_OK)
        return false;
    return true;
}
