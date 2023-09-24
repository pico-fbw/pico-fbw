/**
 * Source file of pico-fbw: https://github.com/MylesAndMore/pico-fbw
 * Licensed under the GNU GPL-3.0
*/

#include <stdbool.h>
#include <stdio.h>
#include "pico/binary_info.h"

#include "hardware/gpio.h"

#include "../io/platform.h"

#include "../lib/semver.h"

#include "config.h"

#include "info.h"

/**
 * Declares all relavent information from pico-fbw into the binary.
 * Must be called once, anywhere in the program.
 * Location does not matter everything is compiled into the binary beforehand.
*/
void info_declare() {
    // There used to be pin defs here before but config is now calculated at runtime so sadly that is now impossible :(

    /* Program info */
    bi_decl(bi_program_description("A fly-by-wire system designed for RC airplanes, for the Rasperry Pi Pico microcontroller."));
    bi_decl(bi_program_version_string(PICO_FBW_VERSION));
    bi_decl(bi_program_url("https://github.com/MylesAndMore/pico-fbw"));
}

int info_checkVersion(const char *version) {
    // Ensure the input version isn't complete garbage
    if (version[0] == '\0') {
        return -2;
    }
    // Parse the version strings into the semantic versioning standard
    semver_t compare;
    if (semver_parse(version, &compare) < 0) {
        semver_free(&compare);
        if (config.debug.debug_fbw) printf("[version] unable to parse input version string!\n");
        return -3;
    }
    semver_t binary;
    if (semver_parse(PICO_FBW_VERSION, &binary) < 0) {
        semver_free(&compare);
        semver_free(&binary);
        if (config.debug.debug_fbw) printf("[version] unable to parse binary version string!\n");
        return -3;
    }
    // Compare the versions
    switch (semver_compare(binary, compare)) {
        case 0:
            return 0; // Equal
        case 1:
            // In most cases this will be a prerelease version but it could also be a user upgrading to a release from prerelease so check that case
            if (binary.prerelease[0] == '\0') {
                return -1; // Lower
            } else {
                if (config.debug.debug_fbw) printf("[version] thanks for testing %s :)\n", binary.prerelease);
                return 1; // Higher
            }
        case -1:
            return -1;
    }
}
