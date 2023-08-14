#ifndef __PICOLITE_H
#define __PICOLITE_H

#define PICOLITE_ERROR_ARGS -1
#define PICOLITE_ERROR_FORMAT -2
#define PICOLITE_ERROR_INCOMPATIBLE -3
#define PICOLITE_ERROR_READ_FAILED -4
#define PICOLITE_ERROR_USB -6
#define PICOLITE_ERROR_NO_DEVICE -7
#define PICOLITE_ERROR_NOT_POSSIBLE -8
#define PICOLITE_ERROR_CONNECTION -9
#define PICOLITE_ERROR_CANCELLED -10
#define PICOLITE_ERROR_VERIFICATION_FAILED -11

#define PICOLITE_ERROR_PERMISSIONS -12
#define PICOLITE_ERROR_DRIVER -13
#define PICOLITE_ERROR_BOOTSEL -14

#define PICOLITE_ERROR_UNKNOWN -99

extern char PICOLITE_ERROR_msg[512];

/**
 * @brief Loads the specified file to any connected Pico, either in BOOTSEL mode or running a compatable program.
 * @param filePath The path to the file to load on the filesystem.
 * 
 * @return 0 on success, -1 to -99 depending on the error.
 * @note Equivalent to "picotool load -u -x <filePath> -f"
 * Requests a reboot after the file is loaded and does not attempt to replace flash sectors with the same data.
*/
int picolite_load(const char *filePath);

#endif // __PICOLITE_H
