#ifndef __INFO_H
#define __INFO_H

#define PICO_FBW_VERSION "1.0.0-beta.2"
#define PICO_FBW_API_VERSION "1.1"
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
 * or -2 if there was an error.
*/
int info_checkVersion(char *version);

#endif // __INFO_H
