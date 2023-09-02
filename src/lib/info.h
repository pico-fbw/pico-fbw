#ifndef __INFO_H
#define __INFO_H

#define PICO_FBW_VERSION "0.0.1-alpha"
#define PICO_FBW_API_VERSION "1.0"
#define WIFLY_VERSION "1.0"

/**
 * Declares all relavent information from pico-fbw into the binary.
 * Must be called once, anywhere in the program.
 * Location does not matter everything is compiled into the binary beforehand.
*/
void info_declare();

#endif // __INFO_H
