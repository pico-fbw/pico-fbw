#ifndef __INFO_H
#define __INFO_H

#define PICO_FBW_VERSION "1.0.0-alpha.3"
#ifdef NDEBUG
    #define DEBUG_BUILD false
#else
    #define DEBUG_BUILD true
#endif

#define PICO_FBW_API_VERSION "1.0"
#define WIFLY_VERSION "1.0"

/**
 * Declares all relavent information from pico-fbw into the binary.
 * Must be called once, anywhere in the program.
 * Location does not matter everything is compiled into the binary beforehand.
*/
void info_declare();

/**
 * Checks the version of the firmware inputted against the current one (compiled into the binary).
 * @param version the version to check against
 * @return 0 if the versions match,
 * 1 if the version is newer (pre-release),
 * -1 if the version is older,
 * -2 if there was no valid version to parse,
 * or -3 if there was a parse error.
*/
int info_checkVersion(const char *version);

#endif // __INFO_H
