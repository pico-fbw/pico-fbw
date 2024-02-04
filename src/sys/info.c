/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
*/

#include <stdbool.h>
#include <stdio.h>
#include "pico/binary_info.h"

#include "hardware/gpio.h"

#include "io/flash.h"
#include "io/platform.h"

#include "lib/semver.h"

#include "sys/info.h"

/**
 * Declares all relavent information from pico-fbw into the binary.
 * Must be called once, anywhere in the program.
 * Location does not matter everything is compiled into the binary beforehand.
*/
void info_declare() {
    /* Program info */
    bi_decl(bi_program_description("pico-fbw is a fly-by-wire and autopilot system for RC airplanes."))
    bi_decl(bi_program_version_string(PICO_FBW_VERSION))
    bi_decl(bi_program_url("https://github.com/pico-fbw/pico-fbw"))
    // There used to be pin defs here before but config is now calculated at runtime so sadly that is now impossible :(
}

int info_checkVersion(const char *version) {
    // Ensure the input version isn't `Absolute garbage!`
    if (version[0] == '\0') return -2;
    // Parse the version strings into the semantic versioning standard
    semver_t compare;
    if (semver_parse(version, &compare) < 0) {
        semver_free(&compare);
        if (print.fbw) printf("[version] unable to parse input version string!\n");
        return -3;
    }
    semver_t binary;
    if (semver_parse(PICO_FBW_VERSION, &binary) < 0) {
        semver_free(&compare);
        semver_free(&binary);
        if (print.fbw) printf("[version] unable to parse binary version string!\n");
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
                if (print.fbw) printf("[version] thanks for testing %s :)\n", binary.prerelease);
                return 1; // Higher
            }
        case -1:
            return -1;
    }
    return -2;
}
